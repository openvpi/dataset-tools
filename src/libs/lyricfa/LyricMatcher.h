#pragma once
/// @file LyricMatcher.h
/// @brief Lyric-to-ASR alignment and matching engine.

#include <QString>
#include <QVector>
#include <memory>
#include "ChineseProcessor.h"
#include "SequenceAligner.h"
#include "SmartHighlighter.h"
#include "LyricData.h"

namespace LyricFA {

    /// @brief Orchestrates lyric file parsing, ASR output processing, and phonetic
    ///        sequence alignment to match ASR results with reference lyrics.
    class LyricMatcher {
    public:
        LyricMatcher();

        /// @brief Parse a lyric file into LyricData.
        /// @param lyric_path Path to the lyric file.
        /// @return Parsed lyric data containing text and phonetic sequences.
        LyricData process_lyric_file(const QString &lyric_path) const;

        /// @brief Extract text and phonetics from ASR .lab output.
        /// @param lab_content Raw content of the .lab file.
        /// @return Pair of (text segments, phonetic segments).
        std::pair<QVector<QString>, QVector<QString>> process_asr_content(const QString &lab_content) const;

        /// @brief Align ASR phonetics against reference lyrics.
        /// @param asr_phonetic Phonetic sequence from ASR output.
        /// @param lyric_text Reference lyric text sequence.
        /// @param lyric_phonetic Reference lyric phonetic sequence.
        /// @return Tuple of (matched_text, matched_phonetic, highlighted_diff).
        std::tuple<QString, QString, QString>
        align_lyric_with_asr(const QVector<QString> &asr_phonetic,
                             const QVector<QString> &lyric_text,
                             const QVector<QString> &lyric_phonetic) const;

        /// @brief Save alignment result to a JSON file.
        /// @param json_path Output JSON file path.
        /// @param text Matched text content.
        /// @param phonetic Matched phonetic content.
        /// @return True on success.
        static bool save_to_json(const QString &json_path, const QString &text, const QString &phonetic);

        /// @brief Access the internal sequence aligner.
        const SequenceAligner& aligner() const { return m_aligner; }
        /// @brief Access the internal smart highlighter.
        const SmartHighlighter& highlighter() const { return m_highlighter; }

    private:
        ChineseProcessor m_processor;   ///< Chinese text processor.
        SequenceAligner m_aligner;      ///< Phonetic sequence aligner.
        SmartHighlighter m_highlighter; ///< Alignment difference highlighter.
    };

} // namespace LyricFA
