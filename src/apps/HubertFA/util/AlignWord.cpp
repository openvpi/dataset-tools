#include "AlignWord.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

namespace HFA {

    Phoneme::Phoneme(const float start, const float end, const std::string &text)
        : start(std::max(0.0f, start)), end(end), text(text) {
        if (!(this->start < this->end)) {
            const std::string error_msg = "Phoneme Invalid: text=" + text + " start=" + std::to_string(this->start) +
                                          ", end=" + std::to_string(this->end);
            throw std::runtime_error(error_msg);
        }
    }

    Word::Word(const float start, const float end, const std::string &text, const bool init_phoneme)
        : start(std::max(0.0f, start)), end(end), text(text) {
        if (!(this->start < this->end)) {
            const std::string error_msg = "Word Invalid: text=" + text + " start=" + std::to_string(this->start) +
                                          ", end=" + std::to_string(this->end);
            throw std::runtime_error(error_msg);
        }

        if (init_phoneme) {
            phonemes.emplace_back(this->start, this->end, this->text);
        }
    }

    float Word::dur() const {
        return end - start;
    }

    void Word::add_phoneme(const Phoneme &phoneme) {
        if (phoneme.start == phoneme.end) {
            const std::string warning_msg = phoneme.text + " phoneme长度为0，非法";
            _add_log("WARNING: " + warning_msg);
            return;
        }
        if (phoneme.start >= start && phoneme.end <= end) {
            phonemes.push_back(phoneme);
        } else {
            const std::string warning_msg = phoneme.text + ": phoneme边界超出word，添加失败";
            _add_log("WARNING: " + warning_msg);
        }
    }

    void Word::append_phoneme(const Phoneme &phoneme) {
        if (phoneme.start == phoneme.end) {
            const std::string warning_msg = phoneme.text + " phoneme长度为0，非法";
            _add_log("WARNING: " + warning_msg);
            return;
        }

        if (phonemes.empty()) {
            if (std::abs(phoneme.start - start) < 1e-6) {
                phonemes.push_back(phoneme);
                end = phoneme.end;
            } else {
                const std::string warning_msg = phoneme.text + ": phoneme左边界超出word，添加失败";
                _add_log("WARNING: " + warning_msg);
            }
        } else {
            if (std::abs(phoneme.start - phonemes.back().end) < 1e-6) {
                phonemes.push_back(phoneme);
                end = phoneme.end;
            } else {
                const std::string warning_msg = phoneme.text + ": phoneme添加失败";
                _add_log("WARNING: " + warning_msg);
            }
        }
    }

    void Word::move_start(float new_start) {
        new_start = std::max(0.0f, new_start);
        if (0 <= new_start && new_start < phonemes[0].end) {
            start = new_start;
            phonemes[0].start = new_start;
        } else {
            const std::string warning_msg = text + ": start >= first_phone_end，无法调整word边界";
            _add_log("WARNING: " + warning_msg);
        }
    }

    void Word::move_end(float new_end) {
        new_end = std::max(0.0f, new_end);
        if (new_end > phonemes.back().start && new_end >= 0) {
            end = new_end;
            phonemes.back().end = new_end;
        } else {
            const std::string warning_msg = text + ": new_end <= first_phone_start，无法调整word边界";
            _add_log("WARNING: " + warning_msg);
        }
    }

    void Word::_add_log(const std::string &message) {
        log.push_back(message);
    }

    std::string Word::get_log() const {
        std::ostringstream oss;
        for (const auto &msg : log) {
            oss << msg << "\n";
        }
        return oss.str();
    }

    void Word::clear_log() {
        log.clear();
    }

    std::vector<std::pair<float, float>>
        WordList::remove_overlapping_intervals(const std::pair<float, float> &raw_interval,
                                               const std::pair<float, float> &remove_interval) {

        float r_start = raw_interval.first;
        float r_end = raw_interval.second;
        const float m_start = remove_interval.first;
        const float m_end = remove_interval.second;

        if (!(r_start < r_end)) {
            throw std::runtime_error("raw_interval.start must be smaller than raw_interval.end");
        }
        if (!(m_start < m_end)) {
            throw std::runtime_error("remove_interval.start must be smaller than remove_interval.end");
        }

        float overlap_start = std::max(r_start, m_start);
        float overlap_end = std::min(r_end, m_end);

        if (overlap_start >= overlap_end) {
            return {raw_interval};
        }

        std::vector<std::pair<float, float>> result;
        if (r_start < overlap_start) {
            result.emplace_back(r_start, overlap_start);
        }

        if (overlap_end < r_end) {
            result.emplace_back(overlap_end, r_end);
        }

        return result;
    }

    void WordList::_add_log(const std::string &message) {
        log_.push_back(message);
    }

    void WordList::sort_by_start() {
        std::sort(words_.begin(), words_.end(), [](const Word &a, const Word &b) { return a.start < b.start; });
    }

    std::vector<Word> WordList::overlapping_words(const Word &new_word) const {
        std::vector<Word> overlapping;
        for (const auto &word : words_) {
            if (!(new_word.end <= word.start || new_word.start >= word.end)) {
                overlapping.push_back(word);
            }
        }
        return overlapping;
    }

    void WordList::append(const Word &word) {
        if (word.phonemes.empty()) {
            const std::string warning_msg = word.text + ": phones为空，非法word";
            _add_log("WARNING: " + warning_msg);
            return;
        }

        if (words_.empty()) {
            words_.push_back(word);
            return;
        }

        if (overlapping_words(word).empty()) {
            words_.push_back(word);
        } else {
            const std::string warning_msg = word.text + ": 区间重叠，无法添加word";
            _add_log("WARNING: " + warning_msg);
        }
    }

    void WordList::add_AP(const Word &new_word, float min_dur) {
        try {
            if (new_word.phonemes.empty()) {
                const std::string warning_msg = new_word.text + " phonemes为空，非法word";
                _add_log("WARNING: " + warning_msg);
                return;
            }

            if (words_.empty()) {
                append(new_word);
                return;
            }

            const auto overlapping = overlapping_words(new_word);
            if (overlapping.empty()) {
                append(new_word);
                sort_by_start();
                return;
            }

            std::vector<std::pair<float, float>> ap_intervals = {
                {new_word.start, new_word.end}
            };

            for (const auto &word : words_) {
                std::vector<std::pair<float, float>> temp_res;
                for (const auto &ap : ap_intervals) {
                    auto intervals = remove_overlapping_intervals(ap, {word.start, word.end});
                    temp_res.insert(temp_res.end(), intervals.begin(), intervals.end());
                }
                ap_intervals = temp_res;
            }

            ap_intervals.erase(std::remove_if(ap_intervals.begin(), ap_intervals.end(),
                                              [min_dur](const std::pair<float, float> &interval) {
                                                  return (interval.second - interval.first) < min_dur;
                                              }),
                               ap_intervals.end());

            for (const auto &[fst, snd] : ap_intervals) {
                try {
                    Word word(fst, snd, new_word.text, true);
                    words_.push_back(word);
                } catch (const std::exception &e) {
                    _add_log("ERROR: " + std::string(e.what()));
                }
            }

            sort_by_start();
        } catch (const std::exception &e) {
            _add_log("ERROR in add_AP: " + std::string(e.what()));
        }
    }

    void WordList::fill_small_gaps(const float wav_length, const float gap_length) {
        try {
            if (empty())
                return;

            if (front().start < 0) {
                words_[0].start = 0;
            }

            if (front().start > 0) {
                if (std::abs(front().start) < gap_length && gap_length < front().dur()) {
                    front().move_start(0);
                }
            }

            if (back().end >= wav_length - gap_length) {
                back().move_end(wav_length);
            }

            for (size_t i = 1; i < words_.size(); i++) {
                if (0 < words_[i].start - words_[i - 1].end && words_[i].start - words_[i - 1].end <= gap_length) {
                    words_[i - 1].move_end(words_[i].start);
                }
            }
        } catch (const std::exception &e) {
            _add_log("ERROR in fill_small_gaps: " + std::string(e.what()));
        }
    }

    void WordList::add_SP(const float wav_length, const std::string &add_phone) {
        try {
            WordList words_res;
            words_res.log_ = log_;

            if (!empty()) {
                if (front().start > 0) {
                    try {
                        words_res.append(Word(0, front().start, add_phone, true));
                    } catch (const std::exception &e) {
                        _add_log("ERROR: " + std::string(e.what()));
                    }
                }

                words_res.append(front());

                for (size_t i = 1; i < words_.size(); i++) {
                    const auto &word = words_[i];
                    if (word.start > words_res.back().end) {
                        try {
                            words_res.append(Word(words_res.back().end, word.start, add_phone, true));
                        } catch (const std::exception &e) {
                            _add_log("ERROR: " + std::string(e.what()));
                        }
                    }
                    words_res.append(word);
                }

                if (back().end < wav_length) {
                    try {
                        words_res.append(Word(back().end, wav_length, add_phone, true));
                    } catch (const std::exception &e) {
                        _add_log("ERROR: " + std::string(e.what()));
                    }
                }
            }

            words_ = std::move(words_res.words_);
            check();
        } catch (const std::exception &e) {
            _add_log("ERROR in add_SP: " + std::string(e.what()));
        }
    }

    std::vector<std::string> WordList::phonemes() const {
        std::vector<std::string> result;
        for (const auto &word : words_) {
            for (const auto &ph : word.phonemes) {
                result.push_back(ph.text);
            }
        }
        return result;
    }

    std::vector<std::pair<float, float>> WordList::intervals() const {
        std::vector<std::pair<float, float>> result;
        result.reserve(words_.size());
        for (const auto &word : words_) {
            result.emplace_back(word.start, word.end);
        }
        return result;
    }

    void WordList::clear_language_prefix() {
        for (auto &word : words_) {
            for (auto &phoneme : word.phonemes) {
                const size_t pos = phoneme.text.find_last_of('/');
                if (pos != std::string::npos) {
                    phoneme.text = phoneme.text.substr(pos + 1);
                }
            }
        }
    }

    bool WordList::check() {
        if (empty()) {
            return true;
        }

        for (size_t i = 0; i < words_.size(); i++) {
            const auto &word = words_[i];

            if (!(word.start < word.end)) {
                const std::string warning_msg = "Word '" + word.text +
                                                "' has invalid time order: start=" + std::to_string(word.start) +
                                                ", end=" + std::to_string(word.end);
                _add_log("WARNING: " + warning_msg);
                return false;
            }

            if (word.phonemes.empty()) {
                const std::string warning_msg = "Word '" + word.text + "' has no phonemes";
                _add_log("WARNING: " + warning_msg);
                return false;
            }

            if (std::abs(word.phonemes[0].start - word.start) > 1e-6) {
                const std::string warning_msg = "Word '" + word.text + "' first phoneme start(" +
                                                std::to_string(word.phonemes[0].start) + ") != word start(" +
                                                std::to_string(word.start) + ")";
                _add_log("WARNING: " + warning_msg);
                return false;
            }

            if (std::abs(word.phonemes.back().end - word.end) > 1e-6) {
                const std::string warning_msg = "Word '" + word.text + "' last phoneme end(" +
                                                std::to_string(word.phonemes.back().end) + ") != word end(" +
                                                std::to_string(word.end) + ")";
                _add_log("WARNING: " + warning_msg);
                return false;
            }

            for (size_t j = 0; j < word.phonemes.size(); j++) {
                if (!(word.phonemes[j].start < word.phonemes[j].end)) {
                    const std::string warning_msg =
                        "Word '" + word.text + "' phoneme '" + word.phonemes[j].text +
                        "' has invalid time order: start=" + std::to_string(word.phonemes[j].start) +
                        ", end=" + std::to_string(word.phonemes[j].end);
                    _add_log("WARNING: " + warning_msg);
                    return false;
                }

                if (j < word.phonemes.size() - 1 &&
                    std::abs(word.phonemes[j].end - word.phonemes[j + 1].start) > 1e-6) {
                    const std::string warning_msg = "Word '" + word.text + "' phoneme '" + word.phonemes[j].text +
                                                    "' end(" + std::to_string(word.phonemes[j].end) +
                                                    ") != next phoneme '" + word.phonemes[j + 1].text + "' start(" +
                                                    std::to_string(word.phonemes[j + 1].start) + ")";
                    _add_log("WARNING: " + warning_msg);
                    return false;
                }
            }
        }

        for (size_t i = 0; i < words_.size() - 1; i++) {
            if (std::abs(words_[i].end - words_[i + 1].start) > 1e-6) {
                const std::string warning_msg = "Word '" + words_[i].text + "' end(" + std::to_string(words_[i].end) +
                                                ") != next word '" + words_[i + 1].text + "' start(" +
                                                std::to_string(words_[i + 1].start) + ")";
                _add_log("WARNING: " + warning_msg);
                return false;
            }
        }

        return true;
    }

    std::string WordList::get_log() const {
        std::ostringstream oss;
        for (const auto &msg : log_) {
            oss << msg << "\n";
        }
        return oss.str();
    }

    void WordList::clear_log() {
        log_.clear();
    }

}