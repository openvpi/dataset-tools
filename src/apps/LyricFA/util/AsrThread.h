#ifndef ASRTHREAD_H
#define ASRTHREAD_H

#include <QRunnable>
#include <QThread>

#include <QSharedPointer>

#include <cpp-pinyin/Pinyin.h>

#include "Asr.h"

namespace LyricFA {
    class AsrThread final : public QObject, public QRunnable {
        Q_OBJECT
    public:
        AsrThread(Asr *asr, QString filename, QString wavPath, QString labPath,
                  const QSharedPointer<Pinyin::Pinyin> &g2p);
        void run() override;

    private:
        Asr *m_asr;
        QString m_filename;
        QString m_wavPath;
        QString m_labPath;
        QSharedPointer<Pinyin::Pinyin> m_g2p = nullptr;

    signals:
        void oneFailed(const QString &filename, const QString &msg);
        void oneFinished(const QString &filename, const QString &msg);
    };
}

#endif // ASRTHREAD_H
