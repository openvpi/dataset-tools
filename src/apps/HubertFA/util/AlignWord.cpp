#include "AlignWord.h"

#include <algorithm>
#include <cmath>
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
        if (!(0 <= start && start < end)) {
            std::cerr << "Warning: Phoneme Invalid: text=" << text << " start=" << start << ", end=" << end
                      << std::endl;
        }
    }

    Word::Word(float start, float end, const std::string &text, const bool init_phoneme)
        : start(std::max(0.0f, start)), end(std::max(0.0f, end)), text(text) {
        if (!(0 <= start && start < end)) {
            std::cerr << "Warning: Word Invalid: text=" << text << " start=" << start << ", end=" << end << std::endl;
        }
        if (init_phoneme) {
            phonemes.emplace_back(start, end, text);
        }
    }

    float Word::dur() const {
        return end - start;
    }

    void Word::add_phoneme(const Phoneme &phoneme) {
        if (phoneme.start == phoneme.end) {
            std::cerr << "Warning: " << phoneme.text << " phoneme长度为0，非法" << std::endl;
            return;
        }
        if (phoneme.start >= start && phoneme.end <= end) {
            phonemes.push_back(phoneme);
        } else {
            std::cerr << "Warning: " << phoneme.text << ": phoneme边界超出word，添加失败" << std::endl;
        }
    }

    void Word::append_phoneme(const Phoneme &phoneme) {
        if (phoneme.start == phoneme.end) {
            std::cerr << "Warning: " << phoneme.text << " phoneme长度为0，非法" << std::endl;
            return;
        }
        if (phonemes.empty()) {
            if (phoneme.start == start) {
                phonemes.push_back(phoneme);
                end = phoneme.end;
            } else {
                std::cerr << "Warning: " << phoneme.text << ": phoneme左边界超出word，添加失败" << std::endl;
            }
        } else {
            if (phoneme.start == phonemes.back().end) {
                phonemes.push_back(phoneme);
                end = phoneme.end;
            } else {
                std::cerr << "Warning: " << phoneme.text << ": phoneme添加失败" << std::endl;
            }
        }
    }

    void Word::move_start(float new_start) {
        new_start = std::max(0.0f, new_start);
        if (0 <= new_start && new_start < phonemes.front().end) {
            start = new_start;
            phonemes.front().start = new_start;
        } else {
            std::cerr << "Warning: " << text << ": start >= first_phone_end，无法调整word边界" << std::endl;
        }
    }

    void Word::move_end(float new_end) {
        new_end = std::max(0.0f, new_end);
        if (new_end > phonemes.back().start && new_end >= 0) {
            end = new_end;
            phonemes.back().end = new_end;
        } else {
            std::cerr << "Warning: " << text << ": new_end <= last_phone_start，无法调整word边界" << std::endl;
        }
    }

    std::vector<Word *> WordList::overlapping_words(const Word *new_word) const {
        std::vector<Word *> result;
        for (const auto &word : *this) {
            if (!word)
                continue;
            if ((word->start < new_word->start && new_word->start < word->end) ||
                (word->start < new_word->end && new_word->end < word->end)) {
                result.push_back(word);
            }
        }
        return result;
    }

    void WordList::append(Word *word) {
        if (word->phonemes.empty()) {
            std::cerr << "Warning: phones为空，非法word" << std::endl;
            return;
        }

        if (this->empty()) {
            this->push_back(word);
            return;
        }

        if (overlapping_words(word).empty()) {
            this->push_back(word);
        } else {
            std::cerr << "Warning: 区间重叠，无法添加word" << std::endl;
        }
    }

    void WordList::add_AP(Word *ap, const float min_dur) {
        if (ap->phonemes.empty()) {
            std::cerr << "Warning: AP phonemes为空，非法word" << std::endl;
            return;
        }

        if (this->empty()) {
            this->append(ap);
            return;
        }

        if (overlapping_words(ap).empty()) {
            this->append(ap);
            this->sort_by_start();
        } else {
            // 检查AP是否包含整个单词
            for (const auto &word : *this) {
                if (ap->start <= word->start && word->end <= ap->end) {
                    std::cerr << "Warning: AP包括整个word，无法添加" << std::endl;
                    return;
                }
            }

            // 调整AP边界
            for (const auto &word : *this) {
                if (word->start <= ap->start && ap->start < word->end) {
                    ap->move_start(word->end);
                }
                if (word->start < ap->end && ap->end <= word->end) {
                    ap->move_end(word->start);
                }
            }

            // 检查调整后的AP是否有效
            if (ap->start < ap->end && ap->dur() > min_dur && overlapping_words(ap).empty()) {
                this->push_back(ap);
                sort_by_start();
            }
        }
    }

    void WordList::fill_small_gaps(const float wav_length, const float gap_length) const {
        if (this->empty())
            return;

        Word *first_word = this->front();
        if (first_word->start < 0) {
            first_word->start = 0;
        }

        if (first_word->start > 0) {
            if (std::fabs(first_word->start) < gap_length && gap_length < first_word->dur()) {
                first_word->move_start(0);
            }
        }

        Word *last_word = this->back();
        if (last_word->end >= wav_length - gap_length) {
            last_word->move_end(wav_length);
        }

        for (size_t i = 1; i < this->size(); i++) {
            Word *prev_word = (*this)[i - 1];
            const Word *curr_word = (*this)[i];
            const float gap = curr_word->start - prev_word->end;
            if (0 < gap && gap <= gap_length) {
                prev_word->move_end(curr_word->start);
            }
        }
    }

    void WordList::add_SP(const float wav_length, const std::string &add_phone) {
        WordList words_res;

        if (!this->empty()) {
            Word *first_word = this->front();
            if (first_word->start > 0) {
                words_res.push_back(new Word(0, first_word->start, add_phone, true));
            }
            words_res.push_back(first_word);

            for (size_t i = 1; i < this->size(); i++) {
                Word *word = (*this)[i];
                if (word->start > words_res.back()->end) {
                    words_res.push_back(new Word(words_res.back()->end, word->start, add_phone, true));
                }
                words_res.push_back(word);
            }

            const Word *last_word = this->back();
            if (last_word->end < wav_length) {
                words_res.push_back(new Word(last_word->end, wav_length, add_phone, true));
            }
        } else {
            // 如果列表为空，添加一个完整的SP单词
            words_res.push_back(new Word(0, wav_length, add_phone, true));
        }

        // 清空当前列表（但不删除内存，因为指针会转移到words_res）
        this->clear();

        // 将结果转移到当前列表
        for (auto word : words_res) {
            this->push_back(word);
        }

        // 清空临时列表（但不删除内存，因为指针已转移）
        words_res.clear();
    }

    float WordList::duration() const {
        if (this->empty())
            return 0.0f;
        return this->back()->end;
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
        std::sort(this->begin(), this->end(), [](const Word *a, const Word *b) { return a->start < b->start; });
    }

} // namespace HFA