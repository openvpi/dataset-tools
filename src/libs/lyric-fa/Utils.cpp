#include "Utils.h"
#include <unordered_set>

namespace LyricFA {

    bool isLetter(const QChar &c) {
        const ushort uc = c.unicode();
        return (uc >= 'a' && uc <= 'z') || (uc >= 'A' && uc <= 'Z');
    }

    bool isSpecialLetter(const QChar &c) {
        static auto specialLetter = QList<QChar>({QChar('\''), QChar('-')});
        for (const QChar &s : specialLetter) {
            if (c == s)
                return true;
        }
        return false;
    }

    bool isHanzi(const QChar &c) {
        const ushort uc = c.unicode();
        return uc >= 0x4e00 && uc <= 0x9fa5;
    }

    bool isKana(const QChar &c) {
        const ushort uc = c.unicode();
        return (uc >= 0x3040 && uc <= 0x309F) || (uc >= 0x30A0 && uc <= 0x30FF);
    }

    bool isDigit(const QChar &c) {
        return c.isDigit();
    }

    bool isSpace(const QChar &c) {
        return c.isSpace();
    }

    bool isSpecialKana(const QChar &c) {
        static const std::unordered_set<ushort> specialKana = {
            0x30E3, 0x30E5, 0x30E7, 0x3083, 0x3085, 0x3087, // ャュョゃゅょ
            0x30A1, 0x30A3, 0x30A5, 0x30A7, 0x30A9,         // ァィゥェォ
            0x3041, 0x3043, 0x3045, 0x3047, 0x3049          // ぁぃぅぇぉ
        };
        return specialKana.find(c.unicode()) != specialKana.end();
    }

    QVector<QString> splitString(const QString &input) {
        QVector<QString> result;
        int pos = 0;
        const int len = input.length();

        while (pos < len) {
            QChar cur = input[pos];
            if (isLetter(cur) || isSpecialLetter(cur)) {
                const int start = pos;
                while (pos < len && (isLetter(input[pos]) || isSpecialLetter(input[pos]))) {
                    ++pos;
                }
                result.append(input.mid(start, pos - start));
            } else if (isHanzi(cur) || isDigit(cur)) {
                result.append(QString(cur));
                ++pos;
            } else if (isKana(cur)) {
                int length = 1;
                if (pos + 1 < len && isSpecialKana(input[pos + 1])) {
                    length = 2;
                }
                result.append(input.mid(pos, length));
                pos += length;
            } else {
                ++pos;
            }
        }
        return result;
    }

} // namespace LyricFA