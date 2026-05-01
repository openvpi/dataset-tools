#pragma once
/// @file HfaTask.h
/// @brief Async task wrapper for HuBERT forced alignment operations.

#include <dsfw/AsyncTask.h>

#include <hubert-infer/Hfa.h>

namespace HFA {
    /// @brief AsyncTask subclass that runs a single HuBERT-FA alignment job in a background thread.
    class HfaThread final : public dstools::AsyncTask {
        Q_OBJECT
    public:
        /// @brief Construct an alignment task.
        /// @param hfa Pointer to the HFA engine instance.
        /// @param filename Display name for the task.
        /// @param wavPath Path to the input WAV file.
        /// @param outTgPath Path to the output TextGrid file.
        /// @param language Target language code.
        /// @param non_speech_ph Non-speech phoneme labels.
        HfaThread(HFA *hfa, QString filename, const QString &wavPath, const QString &outTgPath,
                  const std::string &language, const std::vector<std::string> &non_speech_ph);

    protected:
        /// @brief Execute the alignment task.
        /// @param msg Output message describing the result.
        /// @return True on success, false on failure.
        bool execute(QString &msg) override;

    private:
        HFA *m_hfa;                                ///< HFA engine instance (not owned)
        QString m_wavPath;                         ///< Input WAV file path
        QString m_outTgPath;                       ///< Output TextGrid file path
        std::string m_language;                    ///< Target language code
        std::vector<std::string> m_non_speech_ph;  ///< Non-speech phoneme labels
    };
}
