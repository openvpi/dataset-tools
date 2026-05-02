#include "LyricMatchTask.h"

#include <QApplication>

namespace LyricFA {
    LyricMatchTask::LyricMatchTask(MatchLyric *match, QString filename, QString labPath, QString jsonPath)
        : AsyncTask(std::move(filename)), m_match(match), m_labPath(std::move(labPath)),
          m_jsonPath(std::move(jsonPath)) {
    }

    bool LyricMatchTask::execute(QString &msg) {
        const auto matchRes = m_match->match(identifier(), m_labPath, m_jsonPath, msg);
        return matchRes;
    }

} // LyricFA
