#ifndef TIFA_ALIGNTHREAD_H
#define TIFA_ALIGNTHREAD_H

#include <QObject>
#include <QRunnable>
#include <QString>

#include "Aligner.h"

namespace Tifa {

    /** Aligns one waveform against the text file next to it. */
    class AlignThread final : public QObject, public QRunnable {
        Q_OBJECT
    public:
        AlignThread(const Aligner *aligner, QString filename, QString wavPath, QString outTgPath, AlignRequest request);
        void run() override;

    private:
        const Aligner *m_aligner;
        QString m_filename;
        QString m_wavPath;
        QString m_outTgPath;
        AlignRequest m_request;

    signals:
        void oneFailed(const QString &filename, const QString &msg);
        void oneFinished(const QString &filename, const QString &msg);
    };

} // namespace Tifa

#endif // TIFA_ALIGNTHREAD_H
