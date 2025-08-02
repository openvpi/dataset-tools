#ifndef HFATHREAD_H
#define HFATHREAD_H

#include <QRunnable>
#include <QThread>

#include <QSharedPointer>

#include "HFA.h"

namespace HFA {
    class HfaThread final : public QObject, public QRunnable {
        Q_OBJECT
    public:
        HfaThread(HFA *hfa, QString filename, const QString &wavPath, const QString &outTgPath,
                  const std::string &language, const std::vector<std::string> &non_speech_ph);
        void run() override;

    private:
        HFA *m_hfa;
        QString m_filename;
        QString m_wavPath;
        QString m_outTgPath;
        std::string m_language;
        std::vector<std::string> m_non_speech_ph;

    signals:
        void oneFailed(const QString &filename, const QString &msg);
        void oneFinished(const QString &filename, const QString &msg);
    };
}

#endif // HFATHREAD_H
