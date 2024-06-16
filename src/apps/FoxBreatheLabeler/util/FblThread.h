#ifndef ASRTHREAD_H
#define ASRTHREAD_H

#include <QRunnable>
#include <QThread>

#include <QSharedPointer>

#include "Fbl.h"

namespace FBL {
    class FblThread final : public QObject, public QRunnable {
        Q_OBJECT
    public:
        FblThread(FBL *fbl, QString filename, QString wavPath, QString rawTgPath, QString outTgPath,
                  float ap_threshold = 0.4, float ap_dur = 0.08, float sp_dur = 0.1);
        void run() override;

    private:
        FBL *m_asr;
        QString m_filename;
        QString m_wavPath;
        QString m_rawTgPath;
        QString m_outTgPath;

        float ap_threshold, ap_dur, sp_dur;

    signals:
        void oneFailed(const QString &filename, const QString &msg);
        void oneFinished(const QString &filename, const QString &msg);
    };
}

#endif // ASRTHREAD_H
