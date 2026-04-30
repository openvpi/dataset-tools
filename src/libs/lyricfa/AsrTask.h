#ifndef ASRTHREAD_H
#define ASRTHREAD_H

#include <dsfw/AsyncTask.h>

#include <QSharedPointer>

#include <cpp-pinyin/Pinyin.h>

#include "Asr.h"

namespace LyricFA {
    class AsrThread final : public dstools::AsyncTask {
        Q_OBJECT
    public:
        AsrThread(Asr *asr, QString filename, QString wavPath, QString labPath,
                  const QSharedPointer<Pinyin::Pinyin> &g2p);

    protected:
        bool execute(QString &msg) override;

    private:
        Asr *m_asr;
        QString m_wavPath;
        QString m_labPath;
        QSharedPointer<Pinyin::Pinyin> m_g2p = nullptr;
    };
}

#endif // ASRTHREAD_H
