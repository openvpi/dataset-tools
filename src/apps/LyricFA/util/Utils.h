#ifndef LFA_UTIL_H
#define LFA_UTIL_H

#include <QString>
#include <QVector>

namespace LyricFA {
    bool isLetter(const QChar &c);
    bool isSpecialLetter(const QChar &c);
    bool isHanzi(const QChar &c);
    bool isKana(const QChar &c);
    bool isDigit(const QChar &c);
    bool isSpace(const QChar &c);
    bool isSpecialKana(const QChar &c);

    QVector<QString> splitString(const QString &input);
}

#endif // LFA_UTIL_H