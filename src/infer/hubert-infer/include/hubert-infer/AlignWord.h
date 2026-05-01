/// @file AlignWord.h
/// @brief Word and phone alignment data structures for HuBERT-FA.

#pragma once

#include <hubert-infer/HubertInferGlobal.h>

#include <memory>
#include <string>
#include <vector>

namespace HFA {

    /// @brief A single phoneme with start/end times and text label.
    class HUBERT_INFER_EXPORT Phone {
    public:
        float start;       ///< Start time in seconds.
        float end;         ///< End time in seconds.
        std::string text;  ///< Phoneme label.

        Phone() : start(0), end(0) {}
        /// @brief Constructs a phone with the given time range and label.
        Phone(float start, float end, const std::string &text);
    };

    /// @brief A word containing one or more phones with time boundaries.
    class HUBERT_INFER_EXPORT Word {
    public:
        float start;                    ///< Start time in seconds.
        float end;                      ///< End time in seconds.
        std::string text;               ///< Word text.
        std::vector<Phone> phones;      ///< Constituent phones.
        std::vector<std::string> log;   ///< Diagnostic log messages.

        Word() : start(0), end(0) {}
        /// @brief Constructs a word; optionally initializes a single phone spanning the word.
        Word(float start, float end, const std::string &text, bool init_phone = false);

        /// @brief Returns the word duration in seconds.
        float dur() const;
        /// @brief Adds a phone, replacing phones that overlap.
        void add_phone(const Phone &phone);
        /// @brief Appends a phone to the end of the phone list.
        void append_phone(const Phone &phoneme);
        /// @brief Moves the word start time and adjusts the first phone.
        void move_start(float new_start);
        /// @brief Moves the word end time and adjusts the last phone.
        void move_end(float new_end);

        /// @brief Appends a diagnostic log message.
        void _add_log(const std::string &message);
        /// @brief Returns all log messages as a single string.
        std::string get_log() const;
        /// @brief Clears all log messages.
        void clear_log();
    };

    /// @brief Ordered collection of Words with gap-filling, AP/SP insertion, and validation.
    class HUBERT_INFER_EXPORT WordList {
        std::vector<Word> words_;       ///< Internal word storage.
        std::vector<std::string> log_;  ///< Diagnostic log messages.

        /// @brief Appends a diagnostic log message.
        void _add_log(const std::string &message);
        /// @brief Removes overlapping portions from an interval.
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

        /// @brief Sorts words by start time.
        void sort_by_start();

        /// @brief Returns the end time of the last word, or 0 if empty.
        float duration() const {
            if (!empty())
                return words_.back().end;
            return 0.0;
        }

        /// @brief Returns words that overlap with the given word's time range.
        std::vector<Word> overlapping_words(const Word &new_word) const;
        /// @brief Appends a word to the list.
        void append(const Word &word);
        /// @brief Inserts an AP (aspiration) word, splitting existing words if needed.
        /// @param new_word The AP word to insert.
        /// @param min_dur Minimum duration threshold for insertion.
        void add_AP(const Word &new_word, float min_dur = 0.1f);
        /// @brief Fills small gaps between words with silence.
        /// @param wav_length Total audio length in seconds.
        /// @param gap_length Maximum gap size to fill.
        void fill_small_gaps(float wav_length, float gap_length = 0.1f);
        /// @brief Adds SP (silence) words in gaps between existing words.
        /// @param wav_length Total audio length in seconds.
        /// @param add_phone Phone label for silence words.
        void add_SP(float wav_length, const std::string &add_phone = "SP");

        /// @brief Returns a flat list of all phone labels across words.
        std::vector<std::string> phones() const;
        /// @brief Returns start/end time pairs for all words.
        std::vector<std::pair<float, float>> intervals() const;
        /// @brief Strips language prefix tags from word text.
        void clear_language_prefix();
        /// @brief Validates word/phone boundaries for consistency.
        /// @return True if validation passes.
        bool check();

        /// @brief Returns all log messages as a single string.
        std::string get_log() const;
        /// @brief Clears all log messages.
        void clear_log();
    };

} // namespace HFA

// Backward compatibility alias
namespace HFA { using Phoneme = Phone; }
