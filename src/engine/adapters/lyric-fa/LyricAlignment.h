#pragma once

/// @file LyricAlignment.h
/// @brief Lyric matching chain: ChineseProcessor → LyricMatcher → SmartHighlighter → LyricMatchTask

#include "LyricData.h"
#include "SequenceAligner.h"

#include <QString>
#include <QVector>
#include <cpp-pinyin/Pinyin.h>
#include <dsfw/AsyncTask.h>
#include <dsfw/Result.h>
#include <memory>
#include <tuple>

namespace LyricFA {

    class ChineseProcessor {
    public:
        ChineseProcessor();

        static QString clean_text(const QString &text);
        static QVector<QString> split_text(const QString &text);

        QVector<QString> get_phonetic_list(const QVector<QString> &text_list) const;

    private:
        std::unique_ptr<Pinyin::Pinyin> m_pinyin;
    };

    class SmartHighlighter {
    public:
        explicit SmartHighlighter(const SequenceAligner &aligner);

        std::tuple<QString, QString, QString, int> highlight_differences(const QString &asr_result,
                                                                         const QString &match_phonetic,
                                                                         const QString &match_text) const;

    private:
        const SequenceAligner &m_aligner;
    };

    class LyricMatcher {
    public:
        LyricMatcher();

        dstools::Result<LyricData> process_lyric_file(const QString &lyric_path) const;
        LyricData processLyricContent(const QString &content) const;

        std::pair<QVector<QString>, QVector<QString>> process_asr_content(const QString &lab_content) const;

        std::tuple<QString, QString, QString> align_lyric_with_asr(const QVector<QString> &asr_phonetic,
                                                                   const QVector<QString> &lyric_text,
                                                                   const QVector<QString> &lyric_phonetic) const;

        const SequenceAligner &aligner() const {
            return m_aligner;
        }
        const SmartHighlighter &highlighter() const {
            return m_highlighter;
        }

    private:
        ChineseProcessor m_processor;
        SequenceAligner m_aligner;
        SmartHighlighter m_highlighter;
    };

    class MatchLyric;

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

} // namespace LyricFA
