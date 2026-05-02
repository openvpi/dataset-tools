#pragma once
/// @file LyricMatchTask.h
/// @brief Async task wrapper for lyric matching operations.

#include <dsfw/AsyncTask.h>

#include "MatchLyric.h"

namespace LyricFA {

    /// @brief AsyncTask subclass that runs a single lyric-to-ASR matching job in the background.
    class LyricMatchTask final : public dstools::AsyncTask {
        Q_OBJECT
    public:
        /// @brief Construct a lyric match task.
        /// @param match Pointer to the MatchLyric instance performing the match.
        /// @param filename Name of the lyric file to match against.
        /// @param labPath Path to the ASR .lab file.
        /// @param jsonPath Output path for the alignment JSON result.
        LyricMatchTask(MatchLyric *match, QString filename, QString labPath, QString jsonPath);

    protected:
        /// @brief Execute the lyric matching operation.
        /// @param msg Output message describing success or failure.
        /// @return True if matching succeeded.
        bool execute(QString &msg) override;

    private:
        MatchLyric *m_match;  ///< Lyric matcher instance.
        QString m_labPath;    ///< Path to the ASR .lab file.
        QString m_jsonPath;   ///< Output JSON file path.
    };

} // LyricFA
