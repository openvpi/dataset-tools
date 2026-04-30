#ifndef HFA_H
#define HFA_H

#include <hubert-infer/HubertInferGlobal.h>
#include <hubert-infer/HfaModel.h>

#include <dstools/IInferenceEngine.h>

#include <filesystem>
#include <memory>

#include <hubert-infer/AlignWord.h>
#include <hubert-infer/AlignmentDecoder.h>
#include <hubert-infer/DictionaryG2P.h>
#include <hubert-infer/NonLexicalDecoder.h>

#include <unordered_set>

#include <hubert-infer/Provider.h>

namespace HFA {

    /// HuBERT-based forced alignment engine.
    ///
    /// Wraps an ONNX HuBERT model to perform phoneme-level forced alignment
    /// on audio files. Supports multiple languages via pluggable G2P
    /// dictionaries and handles non-lexical sounds (breath, silence).
    class HUBERT_INFER_EXPORT HFA : public dstools::infer::IInferenceEngine {
    public:
        /// Load the model from @p model_folder using the given execution
        /// provider and device. Check initialized() after construction.
        explicit HFA(const std::filesystem::path &model_folder, ExecutionProvider provider, int device_id);
        ~HFA() override;

        /// Run forced alignment on a single audio file.
        /// @param wavPath      Path to the input WAV file.
        /// @param language      Language code for G2P dictionary selection.
        /// @param non_speech_ph Set of phoneme labels treated as non-speech.
        /// @param[in,out] words Input word/phoneme sequence; output receives
        ///                      aligned time boundaries.
        /// @param[out] msg      Diagnostic message on failure.
        /// @return true on success.
        bool recognize(std::filesystem::path wavPath, const std::string &language,
                       const std::vector<std::string> &non_speech_ph, WordList &words, std::string &msg) const;

        /// Whether the underlying ONNX model was loaded successfully.
        bool initialized() const {
            return m_hfa != nullptr;
        }

        bool isOpen() const override { return initialized(); }
        const char *engineName() const override { return "HuBERT-FA"; }

    private:
        std::unique_ptr<HfaModel> m_hfa;
        std::map<std::string, std::unique_ptr<DictionaryG2P>> m_dictG2p;
        std::unique_ptr<AlignmentDecoder> m_alignmentDecoder;
        std::unique_ptr<NonLexicalDecoder> m_nonLexicalDecoder;
        std::unordered_set<std::string> m_silent_phonemes;

        int hfa_input_sample_rate = 44100;
    };
} // LyricFA

#endif // HFA_H
