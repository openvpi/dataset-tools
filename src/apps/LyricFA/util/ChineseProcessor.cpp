#include "ChineseProcessor.h"
#include "Utils.h"
#include <QRegularExpression>
#include <QStringList>

namespace LyricFA {

    ChineseProcessor::ChineseProcessor() : m_pinyin(std::make_unique<Pinyin::Pinyin>()) {
    }

    QString ChineseProcessor::clean_text(const QString &text) {
        const QRegularExpression re("[^\\u4e00-\\u9fa5]");
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

} // namespace LyricFA