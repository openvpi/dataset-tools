#include "FaTread.h"

#include <QApplication>
#include <QMSystem.h>

namespace LyricFA {
    FaTread::FaTread(MatchLyric *match, QString filename, QString labPath, QString jsonPath, const bool &asr_rectify)
        : m_match(match), m_filename(std::move(filename)), m_labPath(std::move(labPath)),
          m_jsonPath(std::move(jsonPath)), m_asr_rectify(asr_rectify) {
    }

    void FaTread::run() {
        QString matchMsg;
        const auto matchRes = m_match->match(m_filename, m_labPath, m_jsonPath, matchMsg, m_asr_rectify);

        if (!matchRes) {
            Q_EMIT this->oneFailed(m_filename, matchMsg);
            return;
        }
        Q_EMIT this->oneFinished(m_filename, matchMsg);
    }

} // LyricFA