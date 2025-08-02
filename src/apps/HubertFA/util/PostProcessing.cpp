#include "PostProcessing.h"
#include <cmath>
#include <stdexcept>

namespace HFA {

    void fillSmallGaps(const WordList &words, const float wavLength) {
        if (words.empty())
            return;

        if (words[0]->start < 0)
            words[0]->start = 0;


        if (words[0]->start > 0) {
            if (std::fabs(words[0]->start) < MIN_SP_LENGTH && words[0]->dur() > MIN_SP_LENGTH) {
                words[0]->move_start(0);
            }
        }

        if (words.back()->end >= wavLength - MIN_SP_LENGTH)
            words.back()->move_end(wavLength);


        for (size_t i = 1; i < words.size(); ++i) {
            const float gap = words[i]->start - words[i - 1]->end;
            if (gap > 0 && gap <= MIN_SP_LENGTH)
                words[i]->move_start(words[i - 1]->end);
        }
    }

    WordList addSP(const WordList &words, const float wavLength, const std::string &addPhone) {
        WordList wordsRes;

        if (words.empty()) {
            if (wavLength > 0) {
                wordsRes.push_back(new Word(0, wavLength, addPhone, true));
            }
            return wordsRes;
        }

        if (words[0]->start > 0) {
            wordsRes.push_back(new Word(0, words[0]->start, addPhone, true));
        }

        wordsRes.push_back(words[0]);

        for (size_t i = 1; i < words.size(); ++i) {
            if (words[i]->start > wordsRes.back()->end) {
                wordsRes.push_back(new Word(wordsRes.back()->end, words[i]->start, addPhone, true));
            }
            wordsRes.push_back(words[i]);
        }

        if (words.back()->end < wavLength) {
            wordsRes.push_back(new Word(words.back()->end, wavLength, addPhone, true));
        }

        return wordsRes;
    }

} // namespace HFA