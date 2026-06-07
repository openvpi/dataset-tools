#include <dsfw/JsonHelper.h>
#include <dsfw/PathUtils.h>
#include <hubert-infer/Hfa.h>

#include <algorithm>
#include <fstream>

#include <vector>

#include <dsfw/audio/AudioPipeline.h>

#include <iostream>


using dsfw::JsonHelper;

namespace HFA {

    HFA::HFA() = default;

    HFA::HFA(const std::filesystem::path &model_folder, ExecutionProvider provider, int device_id) {
        load(model_folder, provider, device_id);
    }

    HFA::~HFA() = default;

    dsfw::Result<void> HFA::load(const std::filesystem::path &model_folder, ExecutionProvider provider,
                                    int device_id) {
        unload();

        try {
            const std::filesystem::path config_file = model_folder / "config.json";
            auto configResult = JsonHelper::loadFile(config_file);
            if (!configResult) {
                return dsfw::Err(configResult.error());
            }
            auto config = std::move(configResult.value());

            auto melResult = JsonHelper::getRequiredMap<std::string, float>(config, "mel_spec_config");
            if (!melResult) {
                return dsfw::Err(melResult.error());
            }
            auto mel_spec_config = std::move(melResult.value());
            hfa_input_sample_rate = static_cast<int>(mel_spec_config.at("sample_rate"));

            const std::filesystem::path model_path = model_folder / "model.onnx";
            m_hfa = std::make_unique<HfaModel>(model_path, provider, device_id);

            const std::filesystem::path vocab_file = model_folder / "vocab.json";
            auto vocabResult = JsonHelper::loadFile(vocab_file);
            if (!vocabResult) {
                unload();
                return dsfw::Err(vocabResult.error());
            }
            auto vocab = std::move(vocabResult.value());
            const auto dictionaries = JsonHelper::getObject(vocab, "dictionaries");

            for (const auto &[language, dict_node] : dictionaries.items()) {
                if (!dict_node.is_null()) {
                    auto dict_path_str = dict_node.get<std::string>();
                    std::filesystem::path dict_path = model_folder / dict_path_str;

                    if (!std::filesystem::exists(dict_path)) {
                        std::cerr << dsfw::PathUtils::toUtf8(dict_path) << " does not exist" << std::endl;
                    } else {
                        auto dictResult = DictionaryG2P::load(dict_path, language);
                        if (!dictResult) {
                            std::cerr << "Failed to load dictionary: " << dictResult.error() << std::endl;
                        } else {
                            m_dictG2p[language] = std::move(dictResult.value());
                        }
                    }
                }
            }

            auto silentResult = JsonHelper::getRequiredVec<std::string>(vocab, "silent_phonemes");
            if (!silentResult) {
                unload();
                return dsfw::Err(silentResult.error());
            }
            auto silent_phonemes = std::move(silentResult.value());
            m_silent_phonemes = std::unordered_set(silent_phonemes.begin(), silent_phonemes.end());

            auto vocabDictResult = JsonHelper::getRequiredMap<std::string, int>(vocab, "vocab");
            if (!vocabDictResult) {
                unload();
                return dsfw::Err(vocabDictResult.error());
            }
            auto vocab_dict = std::move(vocabDictResult.value());

            auto nonLexResult = JsonHelper::getRequiredVec<std::string>(vocab, "non_lexical_phonemes");
            if (!nonLexResult) {
                unload();
                return dsfw::Err(nonLexResult.error());
            }
            auto non_lexical_phonemes = std::move(nonLexResult.value());
            non_lexical_phonemes.insert(non_lexical_phonemes.begin(), "None");
            m_alignmentDecoder = std::make_unique<AlignmentDecoder>(vocab_dict, mel_spec_config);
            m_nonLexicalDecoder = std::make_unique<NonLexicalDecoder>(non_lexical_phonemes, mel_spec_config);
        } catch (const std::exception &e) {
            unload();
            return dsfw::Err("Failed to load HFA model from " + dsfw::PathUtils::toUtf8(model_folder) + ": " + e.what());
        }

        if (!m_hfa) {
            return dsfw::Err("Cannot load HFA model, check model.onnx and vocab.json");
        }
        return dsfw::Ok();
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

    dsfw::Result<void> HFA::recognize(const std::filesystem::path &wavPath, const std::string &language,
                                         const std::vector<std::string> &non_speech_ph, const std::string &lyricsText,
                                         WordList &words) const {
        if (!m_hfa) {
            return dsfw::Err("HFA model not loaded");
        }

        if (lyricsText.empty()) {
            return dsfw::Err("Empty lyrics text provided for forced alignment"
                                " (audio: " +
                                dsfw::PathUtils::toUtf8(wavPath) + ")");
        }
        if (!std::filesystem::exists(wavPath)) {
            return dsfw::Err("Empty lyrics text provided for forced alignment"
                                " (audio: " +
                                dsfw::PathUtils::toUtf8(wavPath) + ")");
        }

        auto pipeline = dsfw::audio::AudioPipeline::create();
        auto result = pipeline.decodeToMonoFloat(dsfw::PathUtils::toUtf8(wavPath), hfa_input_sample_rate);
        if (!result.ok()) {
            return dsfw::Err("Failed to decode audio: " + result.error() +
                                " (path: " + dsfw::PathUtils::toUtf8(wavPath) + ")");
        }
        auto buffer = result.value();
        auto floats = buffer.floats();
        std::vector<float> audio(floats.begin(), floats.end());

        const auto totalSize = audio.size();
        if (totalSize == 0) {
            return dsfw::Err("Audio file contains 0 samples after resampling, cannot run forced alignment"
                                " (path: " +
                                dsfw::PathUtils::toUtf8(wavPath) + ")");
        }

        const float wav_length = static_cast<float>(audio.size()) / hfa_input_sample_rate;

        if (wav_length > 60) {
            return dsfw::Err(
                "The audio contains continuous pronunciation segments that exceed 60 seconds. Please manually "
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
                return dsfw::Err("Language dictionary not found: " + language);
            }

            std::string alignMsg;
            if (!m_alignmentDecoder->decode(hfaRes.ph_frame_logits, hfaRes.ph_edge_logits, wav_length, ph_seq, words,
                                            alignMsg, word_seq, ph_idx_to_word_idx, true))
                return dsfw::Err(alignMsg);
            const auto word_lists = m_nonLexicalDecoder->decode(hfaRes.cvnt_logits, wav_length, non_speech_ph);

            for (const auto &word_list : word_lists)
                for (const auto &word : word_list)
                    words.add_AP(word);

            words.fill_small_gaps(wav_length);
            words.clear_language_prefix();
            words.add_SP(wav_length, "SP");
            const auto error_log = words.get_log();
            if (!(words.check() && error_log.empty())) {
                return dsfw::Err(error_log);
            }
            return dsfw::Ok();
        }
        return dsfw::Err(modelMsg);
    }

    dsfw::Result<void> HFA::recognize(std::filesystem::path wavPath, const std::string &language,
                                         const std::vector<std::string> &non_speech_ph, WordList &words) const {
        const auto labPath = std::filesystem::path(wavPath).replace_extension(".lab");
        if (!std::filesystem::exists(labPath)) {
            return dsfw::Err("Lab does not exist: " + dsfw::PathUtils::toUtf8(labPath));
        }

        std::ifstream labFile(labPath);
        if (!labFile.is_open()) {
            return dsfw::Err("Failed to open lab file: " + dsfw::PathUtils::toUtf8(labPath));
        }
        std::string labContent((std::istreambuf_iterator<char>(labFile)), std::istreambuf_iterator<char>());
        labFile.close();

        return recognize(wavPath, language, non_speech_ph, labContent, words);
    }
} // namespace HFA
