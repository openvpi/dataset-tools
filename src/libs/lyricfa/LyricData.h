#ifndef LYRICDATA_H
#define LYRICDATA_H

/// @file LyricData.h
/// @brief Data structure for parsed lyric content.

#include <QString>
#include <QVector>

namespace LyricFA {

    /// @brief Holds parsed lyric data with text segments, phonetic transcriptions, and raw text.
    struct LyricData {
        QVector<QString> text_list;     ///< Segmented text pieces.
        QVector<QString> phonetic_list; ///< Corresponding Pinyin phonetics.
        QString raw_text;               ///< Original lyric text.
    };

} // namespace LyricFA

#endif // LYRICDATA_H