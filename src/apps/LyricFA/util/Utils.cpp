#include "Utils.h"

#include <iostream>
#include <unordered_set>

namespace LyricFA {
    bool isLetter(const char16_t &c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    }

    bool isHanzi(const char16_t &c) {
        return c >= 0x4e00 && c <= 0x9fa5;
    }

    bool isKana(const char16_t &c) {
        return (c >= 0x3040 && c <= 0x309F) || (c >= 0x30A0 && c <= 0x30FF);
    }

    bool isDigit(const char16_t &c) {
        return c >= '0' && c <= '9';
    }

    bool isSpace(const char16_t &c) {
        return c == ' ';
    }

    bool isSpecialKana(const char16_t &c) {
        static const std::unordered_set<char16_t> specialKana = {u'ャ', u'ュ', u'ョ', u'ゃ', u'ゅ', u'ょ',
                                                                 u'ァ', u'ィ', u'ゥ', u'ェ', u'ォ', u'ぁ',
                                                                 u'ぃ', u'ぅ', u'ぇ', u'ぉ'};
        return specialKana.find(c) != specialKana.end();
    }

    std::vector<std::u16string> splitString(const std::u16string &input) {
        std::vector<std::u16string> res;
        res.reserve(input.size());
        auto start = input.begin();
        const auto end = input.end();

        while (start != end) {
            const auto &currentChar = *start;
            if (isLetter(currentChar)) {
                auto letterStart = start;
                while (start != end && isLetter(*start)) {
                    ++start;
                }
                res.emplace_back(letterStart, start);
            } else if (isHanzi(currentChar) || isDigit(currentChar) || !isSpace(currentChar)) {
                res.emplace_back(1, currentChar);
                ++start;
            } else if (isKana(currentChar)) {
                const int length = start + 1 != end && isSpecialKana(*(start + 1)) ? 2 : 1;
                res.emplace_back(start, start + length);
                std::advance(start, length);
            } else {
                ++start;
            }
        }
        return res;
    }

}
