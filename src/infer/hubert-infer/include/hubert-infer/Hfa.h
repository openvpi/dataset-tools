#ifndef HFA_H
#define HFA_H

#include <hubert-infer/HubertInferGlobal.h>
#include <hubert-infer/HfaModel.h>

#include <filesystem>
#include <memory>

#include <hubert-infer/AlignWord.h>
#include <hubert-infer/AlignmentDecoder.h>
#include <hubert-infer/DictionaryG2P.h>
#include <hubert-infer/NonLexicalDecoder.h>

#include <unordered_set>

#include <hubert-infer/Provider.h>

namespace HFA {

    class HUBERT_INFER_EXPORT HFA {
    public:
        explicit HFA(const std::filesystem::path &model_folder, ExecutionProvider provider, int device_id);
        ~HFA();

        bool recognize(std::filesystem::path wavPath, const std::string &language,
                       const std::vector<std::string> &non_speech_ph, WordList &words, std::string &msg) const;

        bool initialized() const {
            return m_hfa != nullptr;
        }

    private:
        std::unique_ptr<HfaModel> m_hfa;
        std::map<std::string, DictionaryG2P *> m_dictG2p;
        AlignmentDecoder *m_alignmentDecoder;
        NonLexicalDecoder *m_nonLexicalDecoder;
        std::unordered_set<std::string> m_silent_phonemes;

        int hfa_input_sample_rate = 44100;
    };
} // LyricFA

#endif // HFA_H
