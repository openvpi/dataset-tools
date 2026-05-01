#ifndef ASRTHREAD_H
#define ASRTHREAD_H

/// @file AsrTask.h
/// @brief Async task wrapper for ASR recognition with G2P post-processing.

#include <dsfw/AsyncTask.h>

#include <QSharedPointer>

#include <cpp-pinyin/Pinyin.h>

#include "Asr.h"

namespace LyricFA {
    /// @brief AsyncTask subclass running ASR recognition and Pinyin G2P conversion in background.
    class AsrThread final : public dstools::AsyncTask {
        Q_OBJECT
    public:
        /// @brief Construct an ASR task.
        /// @param asr Pointer to the ASR engine.
        /// @param filename Display name for the task.
        /// @param wavPath Path to the input WAV file.
        /// @param labPath Path to the output label file.
        /// @param g2p Shared Pinyin G2P converter.
        AsrThread(Asr *asr, QString filename, QString wavPath, QString labPath,
                  const QSharedPointer<Pinyin::Pinyin> &g2p);

    protected:
        /// @brief Execute ASR recognition and G2P conversion.
        /// @param msg Error message on failure.
        /// @return True on success.
        bool execute(QString &msg) override;

    private:
        Asr *m_asr;                                  ///< ASR engine pointer.
        QString m_wavPath;                            ///< Input WAV file path.
        QString m_labPath;                            ///< Output label file path.
        QSharedPointer<Pinyin::Pinyin> m_g2p = nullptr; ///< Pinyin G2P converter.
    };
}

#endif // ASRTHREAD_H
