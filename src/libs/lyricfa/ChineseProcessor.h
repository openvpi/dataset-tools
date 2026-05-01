#ifndef CHINESEPROCESSOR_H
#define CHINESEPROCESSOR_H

/// @file ChineseProcessor.h
/// @brief Chinese text processing with Pinyin conversion.

#include <QVector>
#include <cpp-pinyin/Pinyin.h>
#include <memory>

namespace LyricFA {

    /// @brief Processes Chinese text by cleaning, splitting, and converting to Pinyin phonetic representation.
    class ChineseProcessor {
    public:
        ChineseProcessor();

        /// @brief Normalize and clean input text.
        /// @param text Raw input text.
        /// @return Cleaned text.
        static QString clean_text(const QString &text);

        /// @brief Split text into character/word segments.
        /// @param text Input text to split.
        /// @return Vector of text segments.
        static QVector<QString> split_text(const QString &text);

        /// @brief Convert text segments to Pinyin phonetic representations.
        /// @param text_list Segmented text pieces.
        /// @return Corresponding Pinyin phonetics.
        QVector<QString> get_phonetic_list(const QVector<QString> &text_list) const;

    private:
        std::unique_ptr<Pinyin::Pinyin> m_pinyin; ///< Pinyin conversion engine.
    };

} // namespace LyricFA

#endif // CHINESEPROCESSOR_H