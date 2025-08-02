#pragma once

#include <string>
#include <utility>
#include <vector>

namespace HFA {

    class Phoneme {
    public:
        float start;
        float end;
        std::string text;

        Phoneme(float start, float end, const std::string &text);
    };

    class Word {
    public:
        float start;
        float end;
        std::string text;
        std::vector<Phoneme> phonemes;

        Word(float start, float end, const std::string &text, bool init_phoneme = false);

        float dur() const;
        void add_phoneme(const Phoneme &phoneme);
        void append_phoneme(const Phoneme &phoneme);
        void move_start(float new_start);
        void move_end(float new_end);
    };

    class WordList : public std::vector<Word *> {
    public:
        std::vector<Word *> overlapping_words(const Word *new_word) const;
        void append(Word *word);
        void add_AP(Word *ap, float min_dur = 0.1f);

        float duration() const;
        std::vector<std::string> phonemes() const;
        std::vector<std::pair<float, float>> intervals() const;
        void clear_language_prefix() const;

        std::vector<Word *> words() {
            return *this;
        }

        void sort_by_start();
    };

} // namespace HFA