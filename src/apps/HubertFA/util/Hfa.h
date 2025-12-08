#ifndef HFA_H
#define HFA_H

#include "HfaModel.h"

#include <filesystem>
#include <memory>

#include "AlignWord.h"
#include "AlignmentDecoder.h"
#include "DictionaryG2P.h"
#include "NonLexicalDecoder.h"

#include <unordered_set>

#include "Provider.h"

namespace HFA {

    class HFA {
    public:
        explicit HFA(const std::filesystem::path &model_folder, ExecutionProvider provider, int device_id);
        ~HFA();

        bool recognize(std::filesystem::path wavPath, const std::string &language,
                       const std::vector<std::string> &non_speech_ph, WordList &words, std::string &msg) const;

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
