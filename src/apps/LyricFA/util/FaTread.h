#ifndef FATREAD_H
#define FATREAD_H

#include <QRunnable>
#include <QThread>

#include "MatchLyric.h"

namespace LyricFA {

    class FaTread final : public QObject, public QRunnable {
        Q_OBJECT
    public:
        FaTread(MatchLyric *match, QString filename, QString labPath, QString jsonPath, const bool &asr_rectify = true);
        void run() override;

    private:
        MatchLyric *m_match;
        QString m_filename;
        QString m_labPath;
        QString m_jsonPath;
        bool m_asr_rectify;

    signals:
        void oneFailed(const QString &filename, const QString &msg);
        void oneFinished(const QString &filename, const QString &msg);
    };

} // LyricFA

#endif // FATREAD_H
