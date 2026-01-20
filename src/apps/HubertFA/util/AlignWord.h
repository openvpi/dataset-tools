#pragma once

#include <memory>
#include <string>
#include <vector>

namespace HFA {

    class Phone {
    public:
        float start;
        float end;
        std::string text;

        Phone(float start, float end, const std::string &text);
    };

    class Word {
    public:
        float start;
        float end;
        std::string text;
        std::vector<Phone> phones;
        std::vector<std::string> log;

        Word(float start, float end, const std::string &text, bool init_phone = false);

        float dur() const;
        void add_phone(const Phone &phone);
        void append_phone(const Phone &phoneme);
        void move_start(float new_start);
        void move_end(float new_end);

        void _add_log(const std::string &message);
        std::string get_log() const;
        void clear_log();
    };

    class WordList {
        std::vector<Word> words_;
        std::vector<std::string> log_;

        void _add_log(const std::string &message);
        static std::vector<std::pair<float, float>>
            remove_overlapping_intervals(const std::pair<float, float> &raw_interval,
                                         const std::pair<float, float> &remove_interval);

    public:
        WordList() = default;
        WordList(const WordList &) = default;
        WordList &operator=(const WordList &) = default;

        using iterator = std::vector<Word>::iterator;
        using const_iterator = std::vector<Word>::const_iterator;

        iterator begin() {
            return words_.begin();
        }
        iterator end() {
            return words_.end();
        }
        const_iterator begin() const {
            return words_.begin();
        }
        const_iterator end() const {
            return words_.end();
        }
        const_iterator cbegin() const {
            return words_.cbegin();
        }
        const_iterator cend() const {
            return words_.cend();
        }

        size_t size() const {
            return words_.size();
        }
        bool empty() const {
            return words_.empty();
        }
        void clear() {
            words_.clear();
            log_.clear();
        }
        void reserve(size_t n) {
            words_.reserve(n);
        }

        Word &operator[](const size_t idx) {
            return words_[idx];
        }
        const Word &operator[](const size_t idx) const {
            return words_[idx];
        }
        Word &front() {
            return words_.front();
        }
        const Word &front() const {
            return words_.front();
        }
        Word &back() {
            return words_.back();
        }
        const Word &back() const {
            return words_.back();
        }

        void push_back(const Word &word) {
            append(word);
        }
        void pop_back() {
            if (!empty())
                words_.pop_back();
        }

        void sort_by_start();

        float duration() const {
            if (!empty())
                return words_.back().end;
            return 0.0;
        }

        std::vector<Word> overlapping_words(const Word &new_word) const;
        void append(const Word &word);
        void add_AP(const Word &new_word, float min_dur = 0.1f);
        void fill_small_gaps(float wav_length, float gap_length = 0.1f);
        void add_SP(float wav_length, const std::string &add_phone = "SP");

        std::vector<std::string> phones() const;
        std::vector<std::pair<float, float>> intervals() const;
        void clear_language_prefix();
        bool check();

        std::string get_log() const;
        void clear_log();
    };

} // namespace HFA