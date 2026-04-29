#include "LyricMatchTask.h"

#include <QApplication>

namespace LyricFA {
    LyricMatchTask::LyricMatchTask(MatchLyric *match, QString filename, QString labPath, QString jsonPath)
        : m_match(match), m_filename(std::move(filename)), m_labPath(std::move(labPath)),
          m_jsonPath(std::move(jsonPath)) {
    }

    void LyricMatchTask::run() {
        QString matchMsg;
        const auto matchRes = m_match->match(m_filename, m_labPath, m_jsonPath, matchMsg);

        if (!matchRes) {
            Q_EMIT this->oneFailed(m_filename, matchMsg);
            return;
        }
        Q_EMIT this->oneFinished(m_filename, matchMsg);
    }

} // LyricFA