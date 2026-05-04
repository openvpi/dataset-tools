/// @file Hfa.h
/// @brief HuBERT-FA forced alignment engine.

#pragma once
#include <hubert-infer/HubertInferGlobal.h>
#include <hubert-infer/HfaModel.h>

#include <dstools/IInferenceEngine.h>
#include <dstools/Result.h>

#include <filesystem>
#include <memory>

#include <hubert-infer/AlignWord.h>
#include <hubert-infer/AlignmentDecoder.h>
#include <hubert-infer/DictionaryG2P.h>
#include <hubert-infer/NonLexicalDecoder.h>

#include <unordered_set>

#include <hubert-infer/Provider.h>

namespace HFA {

    /// @brief IInferenceEngine implementation combining HfaModel, DictionaryG2P,
    ///        AlignmentDecoder, and NonLexicalDecoder for end-to-end phoneme alignment.
    class HUBERT_INFER_EXPORT HFA : public dstools::infer::IInferenceEngine {
    public:
        HFA();
        /// @brief Constructs and loads the HFA engine from a model folder.
        /// @param model_folder Path to the model directory.
        /// @param provider ONNX Runtime execution provider.
        /// @param device_id Device index for GPU execution.
        explicit HFA(const std::filesystem::path &model_folder, ExecutionProvider provider, int device_id);
        ~HFA() override;

        /// @brief Runs forced alignment on an audio file.
        /// @param wavPath Path to the WAV file.
        /// @param language Language identifier for G2P lookup.
        /// @param non_speech_ph Non-speech phoneme labels to detect.
        /// @param lyricsText Lyrics text for G2P conversion (replaces .lab file reading).
        /// @param[out] words Resulting aligned word list.
        /// @return Success or error.
        dstools::Result<void> recognize(const std::filesystem::path &wavPath, const std::string &language,
                                        const std::vector<std::string> &non_speech_ph,
                                        const std::string &lyricsText, WordList &words) const;

        /// @brief Overload that reads lyrics from a .lab file alongside the audio file.
        /// @deprecated Prefer the overload accepting lyricsText directly.
        dstools::Result<void> recognize(std::filesystem::path wavPath, const std::string &language,
                                        const std::vector<std::string> &non_speech_ph, WordList &words) const;

        /// @brief Returns true if the model is loaded.
        bool initialized() const {
            return m_hfa != nullptr;
        }

        bool isOpen() const override { return initialized(); }
        const char *engineName() const override { return "HuBERT-FA"; }

        /// @brief Loads the model from the given path.
        dstools::Result<void> load(const std::filesystem::path &modelPath, ExecutionProvider provider, int deviceId) override;
        /// @brief Unloads the model and frees resources.
        void unload() override;
        /// @brief Returns estimated GPU/CPU memory usage in bytes.
        int64_t estimatedMemoryBytes() const override;

    private:
        std::unique_ptr<HfaModel> m_hfa;                              ///< ONNX model instance.
        std::map<std::string, std::unique_ptr<DictionaryG2P>> m_dictG2p; ///< Per-language G2P converters.
        std::unique_ptr<AlignmentDecoder> m_alignmentDecoder;          ///< Viterbi alignment decoder.
        std::unique_ptr<NonLexicalDecoder> m_nonLexicalDecoder;        ///< Non-lexical sound detector.
        std::unordered_set<std::string> m_silent_phonemes;             ///< Silent phoneme labels.

        int hfa_input_sample_rate = 44100; ///< Expected input sample rate in Hz.
    };
} // namespace HFA
