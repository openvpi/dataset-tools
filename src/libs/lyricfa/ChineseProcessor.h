#ifndef CHINESEPROCESSOR_H
#define CHINESEPROCESSOR_H

#include <QVector>
#include <cpp-pinyin/Pinyin.h>
#include <memory>

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

} // namespace LyricFA

#endif // CHINESEPROCESSOR_H