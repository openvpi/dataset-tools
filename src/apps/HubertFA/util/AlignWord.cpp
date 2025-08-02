#include "AlignWord.h"

#include <algorithm>
#include <cmath> // 添加cmath头文件用于fabs
#include <iostream>

namespace HFA {
    constexpr float EPS = 1e-6f;

    inline bool f_equal(const float a, const float b) {
        return std::fabs(a - b) < EPS;
    }
    inline bool f_less(const float a, const float b) {
        return a < b - EPS;
    }
    inline bool f_greater(const float a, const float b) {
        return a > b + EPS;
    }
    inline bool f_leq(const float a, const float b) {
        return a <= b + EPS;
    }
    inline bool f_geq(const float a, const float b) {
        return a >= b - EPS;
    }

    Phoneme::Phoneme(const float start, const float end, const std::string &text)
        : start(std::max(0.0f, start)), end(std::max(0.0f, end)), text(text) {
    }

    Word::Word(float start, float end, const std::string &text, const bool init_phoneme)
        : start(std::max(0.0f, start)), end(std::max(0.0f, end)), text(text) {
        if (init_phoneme) {
            phonemes.emplace_back(start, end, text);
        }
    }

    float Word::dur() const {
        return end - start;
    }

    void Word::add_phoneme(const Phoneme &phoneme) {
        if (f_geq(phoneme.start, start) && f_leq(phoneme.end, end)) {
            phonemes.push_back(phoneme);
        } else {
            std::cerr << "Warning: phoneme boundaries exceed word, add failed\n";
        }
    }

    void Word::append_phoneme(const Phoneme &phoneme) {
        if (phonemes.empty()) {
            if (f_equal(phoneme.start, start)) {
                phonemes.push_back(phoneme);
                end = phoneme.end;
            } else {
                std::cerr << "Warning: phoneme left boundary exceeds word, add failed\n";
            }
        } else {
            if (phonemes.empty())
                std::cerr << "Warning: phonemes empty, cannot append\n";

            if (f_equal(phoneme.start, phonemes.back().end)) {
                phonemes.push_back(phoneme);
                end = phoneme.end;
            } else {
                std::cerr << "Warning: phoneme add failed\n";
            }
        }
    }

    void Word::move_start(float new_start) {
        new_start = std::max(0.0f, new_start);
        if (f_geq(new_start, 0) && f_less(new_start, phonemes.front().end)) {
            start = new_start;
            phonemes.front().start = new_start;
        } else {
            std::cerr << "Warning: start >= first_phone_end, cannot adjust word boundary\n";
        }
    }

    void Word::move_end(float new_end) {
        new_end = std::max(0.0f, new_end);
        if (f_greater(new_end, phonemes.back().start) && f_geq(new_end, 0)) {
            end = new_end;
            phonemes.back().end = new_end;
        } else {
            std::cerr << "Warning: new_end <= last_phone_start, cannot adjust word boundary\n";
        }
    }

    std::vector<Word *> WordList::overlapping_words(const Word *new_word) const {
        std::vector<Word *> result;
        for (const auto &word : *this) {
            if ((f_less(word->start, new_word->start) && f_less(new_word->start, word->end)) ||
                (f_less(word->start, new_word->end) && f_less(new_word->end, word->end))) {
                result.push_back(word);
            }
        }
        return result;
    }

    void WordList::append(Word *word) {
        if (word->phonemes.empty()) {
            std::cerr << "Warning: phones empty, illegal word\n";
            return;
        }

        if (this->empty()) {
            this->push_back(word);
            return;
        }

        if (overlapping_words(word).empty()) {
            this->push_back(word);
        } else {
            std::cerr << "Warning: interval overlap, cannot add word\n";
        }
    }

    float WordList::duration() const {
        return this->back()->end;
    }

    void WordList::add_AP(Word *ap, const float min_dur) {
        if (ap->phonemes.empty()) {
            std::cerr << "Warning: phones empty, illegal word\n";
            return;
        }

        if (this->empty()) {
            this->push_back(ap);
            return;
        }

        // 检查AP是否包含整个单词（在整个列表上检查）
        for (const auto &word : *this) {
            if (f_leq(ap->start, word->start) && f_leq(word->end, ap->end)) {
                std::cerr << "Warning: AP includes entire word, cannot add\n";
                return;
            }
        }

        Word *modified_ap = ap;

        // 遍历整个列表调整边界（与Python逻辑一致）
        for (const auto &word : *this) {
            if (f_leq(word->start, modified_ap->start) && f_less(modified_ap->start, word->end)) {
                modified_ap->move_start(word->end);
            }
            if (f_less(word->start, modified_ap->end) && f_leq(modified_ap->end, word->end)) {
                modified_ap->move_end(word->start);
            }
        }

        // 检查调整后的AP是否有效
        if (f_less(modified_ap->start, modified_ap->end) && f_greater(modified_ap->dur(), min_dur) &&
            overlapping_words(modified_ap).empty()) {
            this->push_back(modified_ap);
            sort_by_start();
        }
    }

    std::vector<std::string> WordList::phonemes() const {
        std::vector<std::string> result;
        for (const auto &word : *this) {
            for (const auto &ph : word->phonemes) {
                result.push_back(ph.text);
            }
        }
        return result;
    }

    std::vector<std::pair<float, float>> WordList::intervals() const {
        std::vector<std::pair<float, float>> result;
        for (const auto &word : *this) {
            result.emplace_back(word->start, word->end);
        }
        return result;
    }

    void WordList::clear_language_prefix() const {
        for (auto &word : *this) {
            for (auto &phoneme : word->phonemes) {
                const size_t pos = phoneme.text.find_last_of('/');
                if (pos != std::string::npos) {
                    phoneme.text = phoneme.text.substr(pos + 1);
                }
            }
        }
    }

    void WordList::sort_by_start() {
        std::sort(this->begin(), this->end(), [](const Word *a, const Word *b) { return f_less(a->start, b->start); });
    }

} // namespace HFA