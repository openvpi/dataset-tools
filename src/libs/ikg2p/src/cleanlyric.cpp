#include "cleanlyric.h"
#include "g2pglobal.h"
#include <QDebug>

namespace IKg2p {
    static QStringList orderString(const QString &input) {
        QStringList res;

        // negative lookahead:ッっ;letter,num,chinese,kana
        QRegExp rx("(?![ッっー゜\r(\r\n)])([a-zA-Z]+|[0-9]|[\u4e00-\u9fa5]|[\u3040-\u309F\u30A0-\u30FF]["
                   "ャュョゃゅょァィゥェォぁぃぅぇぉ]?)");

        int pos = 0; // 记录匹配位置的变量

        while ((pos = rx.indexIn(input, pos)) != -1) {
            res.append(input.mid(pos, rx.matchedLength()));
            pos += rx.matchedLength(); // 更新匹配位置
        }
        return res;
    }

    QString CleanLyric::reOrder(const QString &rawStr, QList<int> maxChar) {
        QStringList texts = orderString(rawStr);

        // 按照每行的字符限制重新排列字符
        QStringList newText;
        int t_cursor = 0;
        for (int line = 0; line < maxChar.size(); line++) {
            int charsInLine = maxChar[line];
            for (int i = 0; i < charsInLine; i++) {
                if (t_cursor < texts.size()) {
                    auto t_s = texts.at(t_cursor);
                    t_cursor++;
                    newText.append(t_s);
                } else {
                    break;
                }
            }
            newText.append("\n");
        }

        for (; t_cursor < texts.size(); t_cursor++) {
            newText.append(texts.at(t_cursor));
        }
        return newText.join(" ");
    }

    QStringList CleanLyric::splitLyricAsWord(const QString &rawStr) {
        QStringList res;

        // negative lookahead:ッっ;letter,num,chinese,kana
        QRegExp rx("(?![ッっー゜])([a-zA-Z]+|[0-9]|[\u4e00-\u9fa5]|[\u3040-\u309F\u30A0-\u30FF]["
                   "ャュョゃゅょァィゥェォぁぃぅぇぉ]?|(\n|\r\n))");

        int pos = 0; // 记录匹配位置的变量

        while ((pos = rx.indexIn(rawStr, pos)) != -1) {
            res.append(rawStr.mid(pos, rx.matchedLength()));
            pos += rx.matchedLength(); // 更新匹配位置
        }

        //                qDebug() << res;
        return res;
    }

    QStringList CleanLyric::splitLyricAsChar(const QString &rawStr) {
        QStringList res;

        // negative lookahead:ッっ;letter,num,chinese,kana
        QRegExp rx("(?![ッっー゜])([a-zA-Z]|[0-9]|[\u4e00-\u9fa5]|[\u3040-\u309F\u30A0-\u30FF]["
                   "ャュョゃゅょァィゥェォぁぃぅぇぉ]?)");

        int pos = 0; // 记录匹配位置的变量

        while ((pos = rx.indexIn(rawStr, pos)) != -1) {
            res.append(rawStr.mid(pos, rx.matchedLength()));
            pos += rx.matchedLength(); // 更新匹配位置
        }

        //                qDebug() << res;
        return res;
    }
    QString CleanLyric::filterTextInParentheses(const QString &rawStr) {
        auto res = rawStr;
        QRegExp rx("[{【\\[（\\(].*[\\)）\\]】}]");
        rx.setMinimal(true);
        return res.remove(rx);
    }
}