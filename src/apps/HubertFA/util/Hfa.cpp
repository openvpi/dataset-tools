#include "Hfa.h"

#include "audio-util/Slicer.h"

#include <QBuffer>
#include <QMessageBox>

#include <QDir>
#include <fstream>
#include <iostream>

#include <vector>

#include <audio-util/Util.h>
#include <yaml-cpp/yaml.h>

#include "PostProcessing.h"

namespace fs = std::filesystem;

namespace HFA {

    HFA::HFA(const std::filesystem::path &model_folder, ExecutionProvider provider, int device_id) {
        const fs::path config_file = model_folder / "config.yaml";
        YAML::Node config = YAML::LoadFile(config_file.string());

        const auto melspec_config = config["melspec_config"].as<std::map<std::string, float>>();
        hfa_input_sample_rate = static_cast<int>(melspec_config.find("sample_rate")->second);

        const fs::path encoder_path = model_folder / (config["hubert_config"]["encoder"].as<std::string>() + "-" +
                                                      config["hubert_config"]["channel"].as<std::string>() + ".onnx");
        const fs::path predictor_path = model_folder / "model.onnx";

        m_hfa = std::make_unique<HfaModel>(encoder_path, predictor_path, provider, device_id);

        const fs::path vocab_file = model_folder / "vocab.yaml";
        YAML::Node vocab = YAML::LoadFile(vocab_file.string());
        const auto dictionaries = vocab["dictionaries"];

        for (const auto &entry : dictionaries) {
            const YAML::Node &dict_node = entry.second;
            const auto language = entry.first.as<std::string>();

            if (dict_node && !dict_node.IsNull()) {
                auto dict_path_str = dict_node.as<std::string>();
                fs::path dict_path = model_folder / dict_path_str;

                if (!fs::exists(dict_path)) {
                    std::cerr << dict_path.string() << " does not exist" << std::endl;
                } else {
                    m_dictG2p[entry.first.as<std::string>()] = new DictionaryG2P(dict_path.string(), language);
                }
            }
        }

        const auto silent_phonemes = vocab["silent_phonemes"].as<std::vector<std::string>>();

        m_silent_phonemes = std::unordered_set(silent_phonemes.begin(), silent_phonemes.end());

        const auto vocab_dict = vocab["vocab"].as<std::map<std::string, int>>();
        auto non_speech_phonemes = vocab["non_speech_phonemes"].as<std::vector<std::string>>();
        non_speech_phonemes.insert(non_speech_phonemes.begin(), "None");
        m_alignmentDecoder = new AlignmentDecoder(vocab_dict, non_speech_phonemes, melspec_config);


        if (!m_hfa) {
            qDebug() << "Cannot load ASR Model, there must be files model.onnx and vocab.txt";
        }
    }

    HFA::~HFA() = default;

    bool HFA::recognize(std::filesystem::path wavPath, const std::string &language,
                        const std::vector<std::string> &non_speech_ph, WordList &words, std::string &msg) const {
        if (!m_hfa) {
            return false;
        }

        auto sf_vio = AudioUtil::resample_to_vio(wavPath, msg, 1, hfa_input_sample_rate);

        SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, 1,
                         hfa_input_sample_rate);
        const auto totalSize = sf.frames();

        std::vector<float> audio(totalSize);
        sf.seek(0, SEEK_SET);
        sf.read(audio.data(), static_cast<sf_count_t>(audio.size()));

        const auto wav_length = static_cast<double>(sf.frames()) / hfa_input_sample_rate;

        if (wav_length > 60) {
            msg = "The audio contains continuous pronunciation segments that exceed 60 seconds. Please manually "
                  "segment and rerun the recognition program.";
            return false;
        }

        std::string modelMsg;
        HfaLogits hfaRes;
        if (m_hfa->forward(std::vector<std::vector<float>>{audio}, hfaRes, modelMsg)) {
            const auto labPath = wavPath.replace_extension(".lab");
            if (!fs::exists(labPath)) {
                msg = "Lab does not exist: " + labPath.string();
                return false;
            }

            std::ifstream labFile(labPath);
            if (!labFile.is_open()) {
                msg = "Failed to open lab file: " + labPath.string();
                return false;
            }
            std::string labContent((std::istreambuf_iterator<char>(labFile)), std::istreambuf_iterator<char>());
            labFile.close();

            std::vector<std::string> ph_seq;
            std::vector<std::string> word_seq;
            std::vector<int> ph_idx_to_word_idx;
            if (m_dictG2p.find(language) != m_dictG2p.end()) {
                std::vector<std::string> raw_ph_seq;
                auto &dict = m_dictG2p.find(language)->second;
                std::tie(raw_ph_seq, word_seq, ph_idx_to_word_idx) = dict->convert(labContent, language);
                for (auto &ph : raw_ph_seq) {
                    auto it = std::find(m_silent_phonemes.begin(), m_silent_phonemes.end(), ph);
                    ph_seq.push_back(it == m_silent_phonemes.end() ? language + "/" + ph : ph);
                }
            } else {
                msg = "Language dictionary not found: " + language;
                return false;
            }

            std::vector<float> confidence;
            if (!m_alignmentDecoder->decode(hfaRes.ph_frame_logits, hfaRes.ph_edge_logits, hfaRes.cvnt_logits,
                                            wav_length, ph_seq, words, confidence, msg, word_seq, ph_idx_to_word_idx,
                                            true, non_speech_ph))
                return false;
            fillSmallGaps(words, wav_length);
            words = addSP(words, wav_length, "SP");
            words.clear_language_prefix();
            return true;
        }
        msg = modelMsg;
        return false;
    }
} // LyricFA