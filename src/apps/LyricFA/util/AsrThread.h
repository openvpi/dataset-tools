#ifndef ASRTHREAD_H
#define ASRTHREAD_H

#include <QRunnable>
#include <QThread>

#include "Asr.h"

namespace LyricFA {
    class AsrThread final : public QObject, public QRunnable {
        Q_OBJECT
    public:
        AsrThread(Asr *asr, QString filename, QString wavPath, QString labPath);
        void run() override;

    private:
        Asr *m_asr;
        QString m_filename;
        QString m_wavPath;
        QString m_labPath;

    signals:
        void oneFailed(const QString &filename, const QString &msg);
        void oneFinished(const QString &filename, const QString &msg);
    };
}

#endif // ASRTHREAD_H
