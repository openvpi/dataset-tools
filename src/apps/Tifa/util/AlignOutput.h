#ifndef TIFA_ALIGNOUTPUT_H
#define TIFA_ALIGNOUTPUT_H

#include <cstdint>
#include <string>
#include <vector>

namespace Tifa {

    /** Treatment of tokens the decoder skipped. */
    enum class SkipHandling {
        /** Write nothing when the utterance contains a skipped token. */
        Discard,
        /** Write the remaining tokens, dropping the skipped ones. */
        Omit,
        /** Keep every token, giving each skipped one a millisecond of silence. */
        Preserve,
    };

    /** One decoded alignment, ready to be written as a TextGrid. */
    struct Alignment {
        double timestep = 0.01;
        /** Number of frames of the utterance. */
        int64_t frames = 0;
        /** (onset, offset) frame indices [tokens, 2]. */
        std::vector<int64_t> spans;
        /** Semantic word ID of every token, 1-based. */
        std::vector<int64_t> words;
        /** Pronunciation group ID of every token, 1-based and global. */
        std::vector<int64_t> groups;
        /** Phone label of every token. */
        std::vector<std::string> phonemes;
        /** Labels of the semantic words, indexed by ``word - 1``. */
        std::vector<std::string> texts;
        /** Labels of the pronunciation groups, indexed by ``group - 1``. */
        std::vector<std::string> scripts;

        int64_t tokenCount() const {
            return static_cast<int64_t>(spans.size()) / 2;
        }
    };

    /** Serialized TextGrid of one alignment. */
    struct TextGridResult {
        /** File content; empty when the alignment must not be written. */
        std::string content;
        /** Human readable notes about skipped tokens. */
        std::string report;
    };

    /**
     * Build the three TextGrid tiers of an alignment.
     *
     * Port of ``SaveTextGridCallback``: the ``texts`` tier holds the semantic
     * word intervals, the ``words`` tier the selected pronunciation groups and
     * the ``phones`` tier the individual phonemes. Zero-width spans mark
     * skipped tokens and are discarded, omitted or preserved according to
     * *skipHandling*.
     *
     * @param phonePrefix Language tag stripped from the phone labels, such as
     *                    ``"zh"``; empty disables the stripping.
     */
    TextGridResult buildTextGrid(const Alignment &alignment, SkipHandling skipHandling, const std::string &phonePrefix);

} // namespace Tifa

#endif // TIFA_ALIGNOUTPUT_H
