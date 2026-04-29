#ifndef LYRICMATCHTASK_H
#define LYRICMATCHTASK_H

#include <QRunnable>
#include <QThread>

#include "MatchLyric.h"

namespace LyricFA {

    class LyricMatchTask final : public QObject, public QRunnable {
        Q_OBJECT
    public:
        LyricMatchTask(MatchLyric *match, QString filename, QString labPath, QString jsonPath);
        void run() override;

    private:
        MatchLyric *m_match;
        QString m_filename;
        QString m_labPath;
        QString m_jsonPath;

    signals:
        void oneFailed(const QString &filename, const QString &msg);
        void oneFinished(const QString &filename, const QString &msg);
    };

} // LyricFA

#endif // LYRICMATCHTASK_H
