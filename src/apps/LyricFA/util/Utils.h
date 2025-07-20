#ifndef LFA_UTIL_H
#define LFA_UTIL_H

#include <string>
#include <vector>


namespace LyricFA {
    std::vector<std::u16string> splitString(const std::u16string &input);

    bool isLetter(const char16_t &c);

    bool isHanzi(const char16_t &c);

    bool isKana(const char16_t &c);

    bool isDigit(const char16_t &c);

    bool isSpace(const char16_t &c);

    bool isSpecialKana(const char16_t &c);
}
#endif // LFA_UTIL_H
