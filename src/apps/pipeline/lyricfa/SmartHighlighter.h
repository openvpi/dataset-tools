#ifndef SMARTHIGHLIGHTER_H
#define SMARTHIGHLIGHTER_H

#include <QString>
#include <tuple>
#include "SequenceAligner.h"

namespace LyricFA {

    class SmartHighlighter {
    public:
        explicit SmartHighlighter(const SequenceAligner &aligner);

        std::tuple<QString, QString, QString, int>
        highlight_differences(const QString &asr_result,
                              const QString &match_phonetic,
                              const QString &match_text) const;

    private:
        const SequenceAligner &m_aligner;
    };

} // namespace LyricFA

#endif // SMARTHIGHLIGHTER_H