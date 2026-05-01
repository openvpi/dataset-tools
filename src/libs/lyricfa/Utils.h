#ifndef LFA_UTIL_H
#define LFA_UTIL_H

/// @file Utils.h
/// @brief Character classification and string splitting utilities for lyric processing.

#include <QString>
#include <QVector>

namespace LyricFA {

    /// @brief Check if a character is a Latin letter.
    /// @param c Character to check.
    /// @return True if the character is a Latin letter.
    bool isLetter(const QChar &c);

    /// @brief Check if a character is a special character.
    /// @param c Character to check.
    /// @return True if the character is a special character.
    bool isSpecialLetter(const QChar &c);

    /// @brief Check if a character is a Chinese character (hanzi).
    /// @param c Character to check.
    /// @return True if the character is a Chinese character.
    bool isHanzi(const QChar &c);

    /// @brief Check if a character is a Japanese kana.
    /// @param c Character to check.
    /// @return True if the character is a kana.
    bool isKana(const QChar &c);

    /// @brief Check if a character is a digit.
    /// @param c Character to check.
    /// @return True if the character is a digit.
    bool isDigit(const QChar &c);

    /// @brief Check if a character is whitespace.
    /// @param c Character to check.
    /// @return True if the character is whitespace.
    bool isSpace(const QChar &c);

    /// @brief Check if a character is a special kana.
    /// @param c Character to check.
    /// @return True if the character is a special kana.
    bool isSpecialKana(const QChar &c);

    /// @brief Split a mixed-script string into segments by character type.
    /// @param input Input string to split.
    /// @return Vector of string segments.
    QVector<QString> splitString(const QString &input);

}

#endif // LFA_UTIL_H