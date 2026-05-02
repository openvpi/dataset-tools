#include "LyricMatcher.h"

#include <QFile>
#include <QJsonObject>
#include <QTextStream>

namespace LyricFA {

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

} // namespace LyricFA