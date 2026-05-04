#include "LyricMatcher.h"
#include "LyricMatchTask.h"
#include "SmartHighlighter.h"
#include "ChineseProcessor.h"
#include "Utils.h"

#include <QApplication>
#include <QFile>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

namespace LyricFA {

// ─── ChineseProcessor ────────────────────────────────────────────────────────

    ChineseProcessor::ChineseProcessor() : m_pinyin(std::make_unique<Pinyin::Pinyin>()) {
    }

    QString ChineseProcessor::clean_text(const QString &text) {
        static const QRegularExpression re("[^\\u4e00-\\u9fa5]");
        QString cleaned = text;
        cleaned.remove(re);
        cleaned = cleaned.simplified();
        return cleaned;
    }

    QVector<QString> ChineseProcessor::split_text(const QString &text) {
        return splitString(text);
    }

    QVector<QString> ChineseProcessor::get_phonetic_list(const QVector<QString> &text_list) const {
        QVector<QString> result;
        std::vector<std::string> utf8_token;
        for (const auto &it : text_list)
            utf8_token.push_back(it.toUtf8().constData());
        const auto pinyin_res =
            m_pinyin->hanziToPinyin(utf8_token, Pinyin::ManTone::NORMAL, Pinyin::Error::Default, false, false);
        QStringList syllables;
        for (const auto &item : pinyin_res) {
            syllables << QString::fromStdString(item.pinyin);
        }
        return syllables;
    }

// ─── SmartHighlighter ─────────────────────────────────────────────────────────

    SmartHighlighter::SmartHighlighter(const SequenceAligner &aligner) : m_aligner(aligner) {
    }

    std::tuple<QString, QString, QString, int>
        SmartHighlighter::highlight_differences(const QString &asr_result, const QString &match_phonetic,
                                                const QString &match_text) const {
        QVector<QString> asr_tokens = asr_result.split(' ', Qt::SkipEmptyParts).toVector();
        const QVector<QString> match_phonetic_tokens = match_phonetic.split(' ', Qt::SkipEmptyParts).toVector();
        QVector<QString> match_text_tokens = match_text.split(' ', Qt::SkipEmptyParts).toVector();

        if (match_phonetic_tokens.isEmpty()) {
            QStringList hl_asr;
            for (const QString &t : asr_tokens)
                hl_asr << "(" + t + ")";
            return {hl_asr.join(' '), QString(), QString(), asr_tokens.size()};
        }

        auto [edit_distance, aligned_asr, aligned_match] =
            m_aligner.compute_alignment(asr_tokens, match_phonetic_tokens);

        QStringList asr_highlighted, phonetic_highlighted, text_highlighted;
        int text_index = 0;
        const int text_tokens_len = match_text_tokens.size();

        for (int i = 0; i < aligned_asr.size(); ++i) {
            const QString &asr_token = aligned_asr[i];
            const QString &match_token = aligned_match[i];
            QString text_token;
            if (match_token != "-" && text_index < text_tokens_len) {
                text_token = match_text_tokens[text_index];
                ++text_index;
            }

            if (asr_token == "-") {
                phonetic_highlighted << "(" + match_token + ")";
                if (!text_token.isEmpty())
                    text_highlighted << "(" + text_token + ")";
            } else if (match_token == "-") {
                asr_highlighted << "(" + asr_token + ")";
            } else if (asr_token != match_token) {
                asr_highlighted << "(" + asr_token + ")";
                phonetic_highlighted << "(" + match_token + ")";
                if (!text_token.isEmpty())
                    text_highlighted << "(" + text_token + ")";
            } else {
                asr_highlighted << asr_token;
                phonetic_highlighted << match_token;
                if (!text_token.isEmpty())
                    text_highlighted << text_token;
            }
        }

        return {asr_highlighted.join(' '), phonetic_highlighted.join(' '), text_highlighted.join(' '), edit_distance};
    }

// ─── LyricMatcher ─────────────────────────────────────────────────────────────

    LyricMatcher::LyricMatcher() : m_highlighter(m_aligner) {
    }

    LyricData LyricMatcher::process_lyric_file(const QString &lyric_path) const {
        QFile file(lyric_path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            throw std::runtime_error("Cannot read lyric file");
        }
        QTextStream stream(&file);
        const QString raw_text = stream.readAll();
        file.close();

        const QString cleaned_text = m_processor.clean_text(raw_text);
        const QVector<QString> text_list = m_processor.split_text(cleaned_text);
        const QVector<QString> phonetic_list = m_processor.get_phonetic_list(text_list);
        return {text_list, phonetic_list, cleaned_text};
    }

    std::pair<QVector<QString>, QVector<QString>> LyricMatcher::process_asr_content(const QString &lab_content) const {
        const QString cleaned = m_processor.clean_text(lab_content);
        QVector<QString> text_list = m_processor.split_text(cleaned);
        const QVector<QString> phonetic_list = m_processor.get_phonetic_list(text_list);
        return {text_list, phonetic_list};
    }

    std::tuple<QString, QString, QString>
        LyricMatcher::align_lyric_with_asr(const QVector<QString> &asr_phonetic, const QVector<QString> &lyric_text,
                                           const QVector<QString> &lyric_phonetic) const {
        auto [matched_text, matched_phonetic, start, end, reason] =
            m_aligner.find_best_match_and_return_lyrics(asr_phonetic, lyric_text, lyric_phonetic);
        return {matched_text, matched_phonetic, reason};
    }

    bool LyricMatcher::save_to_json(const QString &json_path, const QString &text, const QString &phonetic) {
        QJsonObject obj;
        obj["raw_text"] = text;
        obj["lab"] = phonetic;
        obj["lab_without_tone"] = phonetic;

        const QJsonDocument doc(obj);
        QFile file(json_path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        return true;
    }

// ─── LyricMatchTask ──────────────────────────────────────────────────────────

    LyricMatchTask::LyricMatchTask(MatchLyric *match, QString filename, QString labPath, QString jsonPath)
        : AsyncTask(std::move(filename)), m_match(match), m_labPath(std::move(labPath)),
          m_jsonPath(std::move(jsonPath)) {
    }

    bool LyricMatchTask::execute(QString &msg) {
        const auto matchRes = m_match->match(identifier(), m_labPath, m_jsonPath, msg);
        return matchRes;
    }

} // LyricFA
