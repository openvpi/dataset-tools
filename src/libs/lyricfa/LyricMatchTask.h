#ifndef LYRICMATCHTASK_H
#define LYRICMATCHTASK_H

#include <dstools/AsyncTask.h>

#include "MatchLyric.h"

namespace LyricFA {

    class LyricMatchTask final : public dstools::AsyncTask {
        Q_OBJECT
    public:
        LyricMatchTask(MatchLyric *match, QString filename, QString labPath, QString jsonPath);

    protected:
        bool execute(QString &msg) override;

    private:
        MatchLyric *m_match;
        QString m_labPath;
        QString m_jsonPath;
    };

} // LyricFA

#endif // LYRICMATCHTASK_H
