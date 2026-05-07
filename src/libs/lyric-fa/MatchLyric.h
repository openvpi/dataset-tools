#pragma once
/// @file MatchLyric.h
/// @brief Batch lyric matching controller.

#include "LyricAlignment.h"
#include <QMap>
#include <QString>
#include <memory>

namespace LyricFA {

    /// @brief Manages a dictionary of reference lyrics and matches individual ASR results against them.
    class MatchLyric {
    public:
        MatchLyric();
        ~MatchLyric();

        /// @brief Load all lyric files from a directory into the internal dictionary.
        /// @param lyric_folder Path to the folder containing lyric files.
        void initLyric(const QString &lyric_folder);

        /// @brief Match a single ASR result against cached lyrics.
        /// @param filename Name of the lyric to match against.
        /// @param labPath Path to the ASR .lab file.
        /// @param jsonPath Output path for the alignment JSON result.
        /// @param msg Output message describing success or failure.
        /// @return True if matching succeeded.
        bool match(const QString &filename, const QString &labPath, const QString &jsonPath, QString &msg) const;

        bool matchText(const QString &filename, const QString &asrText, QString &matchedText, QString &msg) const;

        bool isInitialized() const { return !m_lyricDict.isEmpty(); }

    private:
        /// @brief Cached lyric text and pinyin sequences.
        struct LyricInfo {
            QVector<QString> text;   ///< Lyric text segments.
            QVector<QString> pinyin; ///< Pinyin phonetic sequences.
        };

        QMap<QString, LyricInfo> m_lyricDict;        ///< Dictionary mapping lyric names to cached data.
        std::unique_ptr<LyricMatcher> m_matcher;     ///< Lyric matcher instance.
        int m_diffThreshold = 1;                     ///< Maximum allowed difference count for a valid match.
    };
}
