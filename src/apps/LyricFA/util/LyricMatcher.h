#ifndef LYRICMATCHER_H
#define LYRICMATCHER_H

#include <QString>
#include <QVector>
#include <memory>
#include "ChineseProcessor.h"
#include "SequenceAligner.h"
#include "SmartHighlighter.h"
#include "LyricData.h"

namespace LyricFA {

    class LyricMatcher {
    public:
        LyricMatcher();

        LyricData process_lyric_file(const QString &lyric_path) const;
        std::pair<QVector<QString>, QVector<QString>> process_asr_content(const QString &lab_content) const;

        std::tuple<QString, QString, QString>
        align_lyric_with_asr(const QVector<QString> &asr_phonetic,
                             const QVector<QString> &lyric_text,
                             const QVector<QString> &lyric_phonetic) const;

        static bool save_to_json(const QString &json_path, const QString &text, const QString &phonetic);

        const SequenceAligner& aligner() const { return m_aligner; }
        const SmartHighlighter& highlighter() const { return m_highlighter; }

    private:
        ChineseProcessor m_processor;
        SequenceAligner m_aligner;
        SmartHighlighter m_highlighter;
    };

} // namespace LyricFA

#endif // LYRICMATCHER_H