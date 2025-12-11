#include "Hfa.h"

#include <QBuffer>
#include <QMessageBox>

#include <QDir>
#include <fstream>
#include <iostream>

#include <vector>

#include <audio-util/Util.h>
#include <nlohmann/json.hpp>


namespace fs = std::filesystem;
using json = nlohmann::json;

namespace HFA {

    HFA::HFA(const std::filesystem::path &model_folder, ExecutionProvider provider, int device_id) {
        const fs::path config_file = model_folder / "config.json";
        std::ifstream config_stream(config_file);
        json config = json::parse(config_stream);

        const auto mel_spec_config = config["mel_spec_config"].get<std::map<std::string, float>>();
        hfa_input_sample_rate = static_cast<int>(mel_spec_config.at("sample_rate"));

        const fs::path model_path = model_folder / "model.onnx";
        m_hfa = std::make_unique<HfaModel>(model_path, provider, device_id);

        const fs::path vocab_file = model_folder / "vocab.json";
        std::ifstream vocab_stream(vocab_file);
        json vocab = json::parse(vocab_stream);
        const auto dictionaries = vocab["dictionaries"];

        for (const auto &[language, dict_node] : dictionaries.items()) {
            if (!dict_node.is_null()) {
                auto dict_path_str = dict_node.get<std::string>();
                fs::path dict_path = model_folder / dict_path_str;

                if (!fs::exists(dict_path)) {
                    std::cerr << dict_path.string() << " does not exist" << std::endl;
                } else {
                    m_dictG2p[language] = new DictionaryG2P(dict_path.string(), language);
                }
            }
        }

        const auto silent_phonemes = vocab["silent_phonemes"].get<std::vector<std::string>>();
        m_silent_phonemes = std::unordered_set(silent_phonemes.begin(), silent_phonemes.end());

        const auto vocab_dict = vocab["vocab"].get<std::map<std::string, int>>();
        auto non_lexical_phonemes = vocab["non_lexical_phonemes"].get<std::vector<std::string>>();
        non_lexical_phonemes.insert(non_lexical_phonemes.begin(), "None");
        m_alignmentDecoder = new AlignmentDecoder(vocab_dict, mel_spec_config);
        m_nonLexicalDecoder = new NonLexicalDecoder(non_lexical_phonemes, mel_spec_config);

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

        const float wav_length = static_cast<float>(sf.frames()) / hfa_input_sample_rate;

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

            if (!m_alignmentDecoder->decode(hfaRes.ph_frame_logits, hfaRes.ph_edge_logits, wav_length, ph_seq, words,
                                            msg, word_seq, ph_idx_to_word_idx, true))
                return false;
            const auto word_lists = m_nonLexicalDecoder->decode(hfaRes.cvnt_logits, wav_length, non_speech_ph);

            for (const auto &word_list : word_lists)
                for (const auto &word : word_list)
                    words.add_AP(word);

            words.fill_small_gaps(wav_length);
            words.clear_language_prefix();
            words.add_SP(wav_length, "SP");
            const auto error_log = words.get_log();
            if (!(words.check() && error_log.empty())) {
                msg = error_log;
                return false;
            }
            return true;
        }
        msg = modelMsg;
        return false;
    }
} // namespace HFA