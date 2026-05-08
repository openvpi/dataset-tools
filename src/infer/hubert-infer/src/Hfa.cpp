#include <hubert-infer/Hfa.h>

#include <algorithm>
#include <fstream>
#include <iostream>

#include <vector>

#include <audio-util/Util.h>
#include "JsonUtils.h"


namespace fs = std::filesystem;
using hubert_infer::JsonUtils;

namespace HFA {

    HFA::HFA() = default;

    HFA::HFA(const std::filesystem::path &model_folder, ExecutionProvider provider, int device_id) {
        load(model_folder, provider, device_id);
    }

    HFA::~HFA() = default;

    dstools::Result<void> HFA::load(const std::filesystem::path &model_folder, ExecutionProvider provider, int device_id) {
        unload();

        try {
            std::string jsonErr;

            const fs::path config_file = model_folder / "config.json";
            auto config = JsonUtils::loadFile(config_file, jsonErr);
            if (!jsonErr.empty()) {
                return dstools::Err(jsonErr);
            }

            std::map<std::string, float> mel_spec_config;
            if (!JsonUtils::getRequiredMap<std::string, float>(config, "mel_spec_config", mel_spec_config, jsonErr)) {
                return dstools::Err(jsonErr);
            }
            hfa_input_sample_rate = static_cast<int>(mel_spec_config.at("sample_rate"));

            const fs::path model_path = model_folder / "model.onnx";
            m_hfa = std::make_unique<HfaModel>(model_path, provider, device_id);

            const fs::path vocab_file = model_folder / "vocab.json";
            auto vocab = JsonUtils::loadFile(vocab_file, jsonErr);
            if (!jsonErr.empty()) {
                unload();
                return dstools::Err(jsonErr);
            }
            const auto dictionaries = JsonUtils::getObject(vocab, "dictionaries");

            for (const auto &[language, dict_node] : dictionaries.items()) {
                if (!dict_node.is_null()) {
                    auto dict_path_str = dict_node.get<std::string>();
                    fs::path dict_path = model_folder / dict_path_str;

                    if (!fs::exists(dict_path)) {
                        auto u8path = dict_path.u8string();
                        std::cerr << std::string(u8path.begin(), u8path.end()) << " does not exist" << std::endl;
                    } else {
                        m_dictG2p[language] = std::make_unique<DictionaryG2P>(dict_path, language);
                    }
                }
            }

            std::vector<std::string> silent_phonemes;
            if (!JsonUtils::getRequiredVec<std::string>(vocab, "silent_phonemes", silent_phonemes, jsonErr)) {
                unload();
                return dstools::Err(jsonErr);
            }
            m_silent_phonemes = std::unordered_set(silent_phonemes.begin(), silent_phonemes.end());

            std::map<std::string, int> vocab_dict;
            if (!JsonUtils::getRequiredMap<std::string, int>(vocab, "vocab", vocab_dict, jsonErr)) {
                unload();
                return dstools::Err(jsonErr);
            }
            std::vector<std::string> non_lexical_phonemes;
            if (!JsonUtils::getRequiredVec<std::string>(vocab, "non_lexical_phonemes", non_lexical_phonemes, jsonErr)) {
                unload();
                return dstools::Err(jsonErr);
            }
            non_lexical_phonemes.insert(non_lexical_phonemes.begin(), "None");
            m_alignmentDecoder = std::make_unique<AlignmentDecoder>(vocab_dict, mel_spec_config);
            m_nonLexicalDecoder = std::make_unique<NonLexicalDecoder>(non_lexical_phonemes, mel_spec_config);
        } catch (const std::exception &e) {
            unload();
            return dstools::Err("Failed to load HFA model from " + std::string(model_folder.u8string().begin(), model_folder.u8string().end()) + ": " + e.what());
        }

        if (!m_hfa) {
            return dstools::Err("Cannot load HFA model, check model.onnx and vocab.json");
        }
        return dstools::Ok();
    }

    void HFA::unload() {
        m_hfa.reset();
        m_dictG2p.clear();
        m_alignmentDecoder.reset();
        m_nonLexicalDecoder.reset();
        m_silent_phonemes.clear();
    }

    int64_t HFA::estimatedMemoryBytes() const {
        return initialized() ? 300 * 1024 * 1024LL : 0;
    }

    dstools::Result<void> HFA::recognize(const std::filesystem::path &wavPath, const std::string &language,
                                          const std::vector<std::string> &non_speech_ph,
                                          const std::string &lyricsText, WordList &words) const {
        if (!m_hfa) {
            return dstools::Err("HFA model not loaded");
        }

        if (lyricsText.empty()) {
            return dstools::Err("Empty lyrics text provided for forced alignment"
                " (audio: " + std::string(wavPath.u8string().begin(), wavPath.u8string().end()) + ")");
        }
        if (!fs::exists(wavPath)) {
            return dstools::Err("Empty lyrics text provided for forced alignment"
                " (audio: " + std::string(wavPath.u8string().begin(), wavPath.u8string().end()) + ")");
        }

        std::string msg;
        auto sf_vio = AudioUtil::resample_to_vio(wavPath, msg, 1, hfa_input_sample_rate);

        if (!msg.empty()) {
            return dstools::Err("Failed to resample audio: " + msg
                + " (path: " + wavPath.string() + ")");
        }

        SndfileHandle sf(sf_vio.vio, &sf_vio.data, SFM_READ, SF_FORMAT_WAV | SF_FORMAT_PCM_16, 1,
                         hfa_input_sample_rate);
        if (!sf) {
            return dstools::Err("Failed to open resampled audio for HFA"
                " (path: " + wavPath.string() + ")");
        }
        const auto totalSize = sf.frames();

        if (totalSize == 0) {
            return dstools::Err("Audio file contains 0 samples after resampling, cannot run forced alignment"
                " (path: " + wavPath.string() + ")");
        }

        std::vector<float> audio(totalSize);
        sf.seek(0, SEEK_SET);
        sf.read(audio.data(), static_cast<sf_count_t>(audio.size()));

        const float wav_length = static_cast<float>(sf.frames()) / hfa_input_sample_rate;

        if (wav_length > 60) {
            return dstools::Err("The audio contains continuous pronunciation segments that exceed 60 seconds. Please manually "
                  "segment and rerun the recognition program.");
        }

        std::string modelMsg;
        HfaLogits hfaRes;
        if (m_hfa->forward(std::vector<std::vector<float>>{audio}, hfaRes, modelMsg)) {
            std::vector<std::string> ph_seq;
            std::vector<std::string> word_seq;
            std::vector<int> ph_idx_to_word_idx;
            if (m_dictG2p.find(language) != m_dictG2p.end()) {
                std::vector<std::string> raw_ph_seq;
                auto &dict = m_dictG2p.find(language)->second;
                std::tie(raw_ph_seq, word_seq, ph_idx_to_word_idx) = dict->convert(lyricsText, language);
                for (auto &ph : raw_ph_seq) {
                    auto it = std::find(m_silent_phonemes.begin(), m_silent_phonemes.end(), ph);
                    ph_seq.push_back(it == m_silent_phonemes.end() ? language + "/" + ph : ph);
                }
            } else {
                return dstools::Err("Language dictionary not found: " + language);
            }

            if (!m_alignmentDecoder->decode(hfaRes.ph_frame_logits, hfaRes.ph_edge_logits, wav_length, ph_seq, words,
                                            msg, word_seq, ph_idx_to_word_idx, true))
                return dstools::Err(msg);
            const auto word_lists = m_nonLexicalDecoder->decode(hfaRes.cvnt_logits, wav_length, non_speech_ph);

            for (const auto &word_list : word_lists)
                for (const auto &word : word_list)
                    words.add_AP(word);

            words.fill_small_gaps(wav_length);
            words.clear_language_prefix();
            words.add_SP(wav_length, "SP");
            const auto error_log = words.get_log();
            if (!(words.check() && error_log.empty())) {
                return dstools::Err(error_log);
            }
            return dstools::Ok();
        }
        return dstools::Err(modelMsg);
    }

    dstools::Result<void> HFA::recognize(std::filesystem::path wavPath, const std::string &language,
                                          const std::vector<std::string> &non_speech_ph, WordList &words) const {
        const auto labPath = std::filesystem::path(wavPath).replace_extension(".lab");
        if (!fs::exists(labPath)) {
            return dstools::Err("Lab does not exist: " + labPath.string());
        }

        std::ifstream labFile(labPath);
        if (!labFile.is_open()) {
            return dstools::Err("Failed to open lab file: " + labPath.string());
        }
        std::string labContent((std::istreambuf_iterator<char>(labFile)), std::istreambuf_iterator<char>());
        labFile.close();

        return recognize(wavPath, language, non_speech_ph, labContent, words);
    }
} // namespace HFA
