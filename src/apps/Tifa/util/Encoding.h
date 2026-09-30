#ifndef TIFA_ENCODING_H
#define TIFA_ENCODING_H

#include <cstdint>
#include <string>
#include <vector>

#include "G2P.h"
#include "Vocabulary.h"

namespace Tifa {

    /**
     * Treatment of phonemes that the vocabulary does not know.
     *
     * TIFA knows three modes, of which two differ visibly: "raise" and
     * "discard" both reject the utterance and only change how it is reported,
     * while "force" removes the affected pronunciation and keeps the rest.
     */
    enum class OovHandling {
        /** Fail the utterance (TIFA's "raise" and "discard"). */
        Raise,
        /** Drop the whole pronunciation and continue (TIFA's "force"). */
        Force,
    };

    /** One complete pronunciation of a word, as shown in the alignment report. */
    struct CandidateInfo {
        int reading = 0;
        std::vector<std::string> phonemes;
        std::vector<std::string> scripts;
    };

    /**
     * Whole-word candidate grids of one utterance.
     *
     * Port of ``encode_paths``/``_pack_paths``: every pronunciation of a word is
     * one candidate column, the rows hold the alignment of the candidates of
     * that word, and ``words`` marks which word owns which row. Token ``0``
     * marks a row position that a candidate does not use.
     *
     * The buffers map directly onto the tensor inputs of the exported graphs:
     * ``tokens`` is ``paths`` [1, capacity, width], ``words`` is ``words``
     * [1, capacity], ``groups`` is ``groups`` [1, capacity, width] and
     * ``candidates`` is the boolean mask [1, wordCount, width].
     */
    struct PathGrid {
        int64_t capacity = 0;
        int64_t width = 0;
        int64_t wordCount = 0;
        std::vector<int64_t> tokens;
        std::vector<int64_t> groups;
        std::vector<int64_t> words;
        std::vector<uint8_t> candidates;
        /** Non-empty candidates of every word, aligned with the mask columns. */
        std::vector<std::vector<CandidateInfo>> lexicon;
        /** Texts of the words, as produced by G2P. */
        std::vector<std::string> texts;

        int64_t tokenAt(int64_t row, int64_t column) const {
            return tokens[row * width + column];
        }
    };

    /**
     * Encode G2P words into candidate grids.
     *
     * @param languages Active language tags, used to resolve the language of
     *                  converters that defer to the vocabulary.
     * @return False on unknown phonemes or on a word without a pronunciation;
     *         *error* then carries the report of TIFA's G2PEncodingError.
     *         Structural problems (a reserved token, an empty group) are
     *         reported the same way but must not be treated as out of vocabulary.
     */
    bool encodePaths(const std::vector<G2PWord> &words, const Vocabulary &vocabulary, OovHandling oovHandling,
                     const std::vector<std::string> &languages, PathGrid &grid, std::string &error);

    /**
     * ``first_choices``: the first valid candidate of every word, 1-based, or
     * zero for a word that has no usable pronunciation.
     */
    void firstChoices(const PathGrid &grid, std::vector<int64_t> &choices);

} // namespace Tifa

#endif // TIFA_ENCODING_H
