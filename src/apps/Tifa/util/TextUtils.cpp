#include "TextUtils.h"

namespace Tifa::Text {

    std::u32string decode(const std::string &utf8) {
        std::u32string result;
        result.reserve(utf8.size());
        size_t i = 0;
        while (i < utf8.size()) {
            const auto byte = static_cast<unsigned char>(utf8[i]);
            char32_t cp = 0xFFFD;
            size_t length = 1;
            if (byte < 0x80) {
                cp = byte;
            } else if ((byte & 0xE0) == 0xC0) {
                cp = byte & 0x1F;
                length = 2;
            } else if ((byte & 0xF0) == 0xE0) {
                cp = byte & 0x0F;
                length = 3;
            } else if ((byte & 0xF8) == 0xF0) {
                cp = byte & 0x07;
                length = 4;
            }
            bool valid = i + length <= utf8.size();
            if (valid) {
                for (size_t k = 1; k < length; ++k) {
                    const auto continuation = static_cast<unsigned char>(utf8[i + k]);
                    if ((continuation & 0xC0) != 0x80) {
                        valid = false;
                        break;
                    }
                    cp = (cp << 6) | (continuation & 0x3F);
                }
            }
            if (valid) {
                result.push_back(cp);
                i += length;
            } else {
                result.push_back(0xFFFD);
                i += 1;
            }
        }
        return result;
    }

    std::string encode(const std::u32string &text) {
        std::string result;
        result.reserve(text.size() * 3);
        for (const char32_t cp : text) {
            if (cp < 0x80) {
                result.push_back(static_cast<char>(cp));
            } else if (cp < 0x800) {
                result.push_back(static_cast<char>(0xC0 | (cp >> 6)));
                result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else if (cp < 0x10000) {
                result.push_back(static_cast<char>(0xE0 | (cp >> 12)));
                result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            } else {
                result.push_back(static_cast<char>(0xF0 | (cp >> 18)));
                result.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
                result.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
            }
        }
        return result;
    }

    bool isSpace(char32_t c) {
        switch (c) {
            case U' ':
            case U'\t':
            case U'\n':
            case U'\r':
            case U'\v':
            case U'\f':
            case 0x0085:
            case 0x00A0:
            case 0x1680:
            case 0x2028:
            case 0x2029:
            case 0x202F:
            case 0x205F:
            case 0x3000:
                return true;
            default:
                return c >= 0x2000 && c <= 0x200A;
        }
    }

    bool isKana(char32_t c) {
        return (c >= 0x3040 && c <= 0x309F) || (c >= 0x30A0 && c <= 0x30FF);
    }

    bool isSmallKana(char32_t c) {
        switch (c) {
            case 0x3083: // ゃ
            case 0x3085: // ゅ
            case 0x3087: // ょ
            case 0x30E3: // ャ
            case 0x30E5: // ュ
            case 0x30E7: // ョ
            case 0x3041: // ぁ
            case 0x3043: // ぃ
            case 0x3045: // ぅ
            case 0x3047: // ぇ
            case 0x3049: // ぉ
            case 0x30A1: // ァ
            case 0x30A3: // ィ
            case 0x30A5: // ゥ
            case 0x30A7: // ェ
            case 0x30A9: // ォ
                return true;
            default:
                return false;
        }
    }

    bool isCjk(char32_t c) {
        if (c >= 0x2E80 && c <= 0x2EFF) {
            return true;
        }
        if (c >= 0x2F00 && c <= 0x2FDF) {
            return true;
        }
        if (c >= 0x3000 && c <= 0x303F) {
            return true;
        }
        if (isKana(c)) {
            return true;
        }
        if (c >= 0x31F0 && c <= 0x31FF) {
            return true;
        }
        if (c >= 0x3400 && c <= 0x4DBF) {
            return true;
        }
        if (c >= 0x4E00 && c <= 0x9FFF) {
            return true;
        }
        if (c >= 0xAC00 && c <= 0xD7AF) {
            return true;
        }
        if (c >= 0xF900 && c <= 0xFAFF) {
            return true;
        }
        return c >= 0xFF00 && c <= 0xFFEF;
    }

    bool isHanzi(char32_t c) {
        return c >= 0x4E00 && c <= 0x9FA5;
    }

    bool isJapaneseChar(char32_t c) {
        if (isKana(c)) {
            return true;
        }
        switch (c) {
            case 0x3005: // 々
            case 0x3006: // 〆
            case 0x3007: // 〇
                return true;
            default:
                break;
        }
        if (c >= 0x3400 && c <= 0x4DBF) {
            return true;
        }
        if (c >= 0x4E00 && c <= 0x9FFF) {
            return true;
        }
        if (c >= 0xF900 && c <= 0xFAFF) {
            return true;
        }
        if (c >= 0x20000 && c <= 0x2FA1F) {
            return true;
        }
        return c >= 0x30000 && c <= 0x323AF;
    }

    bool isPunctuation(char32_t c) {
        // ASCII punctuation, except apostrophes and hyphens.
        if (c < 0x80) {
            if (c == U'\'' || c == U'-') {
                return false;
            }
            return (c >= 0x21 && c <= 0x2F) || (c >= 0x3A && c <= 0x40) || (c >= 0x5B && c <= 0x60) ||
                   (c >= 0x7B && c <= 0x7E);
        }
        switch (c) {
            case 0xFF0C: // ，
            case 0x3002: // 。
            case 0xFF1B: // ；
            case 0xFF1A: // ：
            case 0x201C: // “
            case 0x201D: // ”
            case 0x2018: // ‘
            case 0x2019: // ’
            case 0xFF08: // （
            case 0xFF09: // ）
            case 0x3010: // 【
            case 0x3011: // 】
            case 0x300A: // 《
            case 0x300B: // 》
            case 0x2026: // …
            case 0x2014: // —
            case 0xFF5E: // ～
            case 0x3001: // 、
            case 0x00B7: // ·
            case 0xFF01: // ！
            case 0xFF1F: // ？
                return true;
            default:
                return false;
        }
    }

    char32_t toLower(char32_t c) {
        // The preprocessor of TIFA lower cases with Python's str.lower(). This
        // covers ASCII, Latin-1, Latin Extended-A/B, Greek and Cyrillic, where
        // the lower case is the upper case plus one offset; the remaining
        // scripts keep their case.
        if (c >= U'A' && c <= U'Z') {
            return c + 0x20;
        }
        if ((c >= 0x00C0 && c <= 0x00DE && c != 0x00D7) || // Latin-1 supplement
            (c >= 0x0391 && c <= 0x03AB && c != 0x03A2) || // Greek
            (c >= 0x0410 && c <= 0x042F) ||                // Cyrillic
            (c >= 0xFF21 && c <= 0xFF3A)) {                // Full width Latin
            return c + 0x20;
        }
        if (c == 0x0130) { // İ
            return U'i';
        }
        if (c >= 0x0400 && c <= 0x040F) { // Cyrillic supplementary
            return c + 0x50;
        }
        if ((c >= 0x0100 && c <= 0x0137 && c % 2 == 0) || // Latin Extended-A
            (c >= 0x0139 && c <= 0x0148 && c % 2 == 1) || (c >= 0x014A && c <= 0x0177 && c % 2 == 0) ||
            (c >= 0x0179 && c <= 0x017E && c % 2 == 1)) { // Latin Extended-B
            return c + 1;
        }
        return c;
    }

    std::u32string lower(const std::u32string &text) {
        std::u32string result;
        result.reserve(text.size());
        for (const char32_t c : text) {
            result.push_back(toLower(c));
        }
        return result;
    }

    std::vector<std::u32string> lower(const std::vector<std::u32string> &tokens) {
        std::vector<std::u32string> result;
        result.reserve(tokens.size());
        for (const std::u32string &token : tokens) {
            result.push_back(lower(token));
        }
        return result;
    }

    std::vector<Range> wordSpans(const std::u32string &text) {
        std::vector<Range> spans;
        size_t i = 0;
        while (i < text.size()) {
            if (isSpace(text[i])) {
                i += 1;
                continue;
            }
            const size_t begin = i;
            if (isKana(text[i]) && i + 1 < text.size() && isSmallKana(text[i + 1])) {
                i += 2;
            } else if (isCjk(text[i])) {
                i += 1;
            } else {
                i += 1;
                while (i < text.size() && !isSpace(text[i]) && !isCjk(text[i])) {
                    i += 1;
                }
            }
            spans.emplace_back(begin, i);
        }
        return spans;
    }

    std::vector<std::u32string> splitWords(const std::u32string &text) {
        std::vector<std::u32string> words;
        for (const auto &[begin, end] : wordSpans(text)) {
            words.push_back(text.substr(begin, end - begin));
        }
        return words;
    }

    bool findRun(const std::u32string &text, const std::function<bool(char32_t)> &accepts, size_t &begin, size_t &end) {
        for (size_t i = 0; i < text.size(); ++i) {
            if (!accepts(text[i])) {
                continue;
            }
            size_t last = i + 1;
            while (last < text.size() && accepts(text[last])) {
                last += 1;
            }
            begin = i;
            end = last;
            return true;
        }
        return false;
    }

    std::vector<std::u32string> filterPunctuation(const std::vector<std::u32string> &tokens) {
        std::vector<std::u32string> result;
        for (const auto &token : tokens) {
            std::u32string part;
            for (const char32_t c : token) {
                if (isPunctuation(c)) {
                    if (!part.empty()) {
                        result.push_back(part);
                        part.clear();
                    }
                } else {
                    part.push_back(c);
                }
            }
            if (!part.empty()) {
                result.push_back(part);
            }
        }
        return result;
    }

    std::vector<std::u32string> stripWhitespace(const std::vector<std::u32string> &tokens) {
        std::vector<std::u32string> result;
        for (const auto &token : tokens) {
            size_t begin = 0;
            size_t end = token.size();
            while (begin < end && isSpace(token[begin])) {
                begin += 1;
            }
            while (end > begin && isSpace(token[end - 1])) {
                end -= 1;
            }
            if (begin < end) {
                result.push_back(token.substr(begin, end - begin));
            }
        }
        return result;
    }

} // namespace Tifa::Text
