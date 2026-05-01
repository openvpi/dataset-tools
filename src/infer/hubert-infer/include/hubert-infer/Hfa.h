#ifndef HFA_H
#define HFA_H

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

    class HUBERT_INFER_EXPORT HFA : public dstools::infer::IInferenceEngine {
    public:
        HFA();
        explicit HFA(const std::filesystem::path &model_folder, ExecutionProvider provider, int device_id);
        ~HFA() override;

        dstools::Result<void> recognize(std::filesystem::path wavPath, const std::string &language,
                                        const std::vector<std::string> &non_speech_ph, WordList &words) const;

        bool initialized() const {
            return m_hfa != nullptr;
        }

        bool isOpen() const override { return initialized(); }
        const char *engineName() const override { return "HuBERT-FA"; }

        dstools::Result<void> load(const std::filesystem::path &modelPath, ExecutionProvider provider, int deviceId) override;
        void unload() override;
        int64_t estimatedMemoryBytes() const override;

    private:
        std::unique_ptr<HfaModel> m_hfa;
        std::map<std::string, std::unique_ptr<DictionaryG2P>> m_dictG2p;
        std::unique_ptr<AlignmentDecoder> m_alignmentDecoder;
        std::unique_ptr<NonLexicalDecoder> m_nonLexicalDecoder;
        std::unordered_set<std::string> m_silent_phonemes;

        int hfa_input_sample_rate = 44100;
    };
} // namespace HFA

#endif // HFA_H
