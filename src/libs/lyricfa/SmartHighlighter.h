#ifndef SMARTHIGHLIGHTER_H
#define SMARTHIGHLIGHTER_H

/// @file SmartHighlighter.h
/// @brief Alignment difference highlighter for visual diff display.

#include <QString>
#include <tuple>
#include "SequenceAligner.h"

namespace LyricFA {

    /// @brief Highlights differences between ASR output and matched lyrics for visual feedback.
    class SmartHighlighter {
    public:
        /// @brief Construct a highlighter with a reference to a sequence aligner.
        /// @param aligner Sequence aligner used for diff computation.
        explicit SmartHighlighter(const SequenceAligner &aligner);

        /// @brief Highlight differences between ASR result and matched lyrics.
        /// @param asr_result ASR output text.
        /// @param match_phonetic Matched phonetic sequence.
        /// @param match_text Matched lyric text.
        /// @return Tuple of (highlighted_asr, highlighted_phonetic, highlighted_text, diff_count).
        std::tuple<QString, QString, QString, int>
        highlight_differences(const QString &asr_result,
                              const QString &match_phonetic,
                              const QString &match_text) const;

    private:
        const SequenceAligner &m_aligner; ///< Sequence aligner for diff computation.
    };

} // namespace LyricFA

#endif // SMARTHIGHLIGHTER_H