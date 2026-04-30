#ifndef HFATHREAD_H
#define HFATHREAD_H

#include <dstools/AsyncTask.h>

#include <hubert-infer/Hfa.h>

namespace HFA {
    class HfaThread final : public dstools::AsyncTask {
        Q_OBJECT
    public:
        HfaThread(HFA *hfa, QString filename, const QString &wavPath, const QString &outTgPath,
                  const std::string &language, const std::vector<std::string> &non_speech_ph);

    protected:
        bool execute(QString &msg) override;

    private:
        HFA *m_hfa;
        QString m_wavPath;
        QString m_outTgPath;
        std::string m_language;
        std::vector<std::string> m_non_speech_ph;
    };
}

#endif // HFATHREAD_H
