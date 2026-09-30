#ifndef TIFA_TEXTUTILS_H
#define TIFA_TEXTUTILS_H

#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace Tifa::Text {

    /** UTF-8 to UTF-32 conversion. Invalid bytes become U+FFFD. */
    std::u32string decode(const std::string &utf8);

    /** UTF-32 to UTF-8 conversion. */
    std::string encode(const std::u32string &text);

    bool isSpace(char32_t c);
    bool isKana(char32_t c);
    bool isSmallKana(char32_t c);

    /** Characters treated as word separators by the G2P word splitting rules. */
    bool isCjk(char32_t c);

    /** Hanzi range used by the Chinese G2P converters. */
    bool isHanzi(char32_t c);

    /** Characters claimed by the Japanese converters (kana and kanji). */
    bool isJapaneseChar(char32_t c);

    /** Punctuation discarded by the filter-punctuation preprocessor. */
    bool isPunctuation(char32_t c);

    char32_t toLower(char32_t c);

    /** Lowercase ASCII and Latin-1 letters, leaving all other characters unchanged. */
    std::u32string lower(const std::u32string &text);

    /** Lowercase every token, mirroring the lowercase preprocessor. */
    std::vector<std::u32string> lower(const std::vector<std::u32string> &tokens);

    using Range = std::pair<size_t, size_t>;

    /**
     * Split on whitespace and CJK characters, retaining kana digraphs.
     * Ranges are half-open code point offsets.
     */
    std::vector<Range> wordSpans(const std::u32string &text);

    std::vector<std::u32string> splitWords(const std::u32string &text);

    /** Find the first contiguous run of accepted characters. */
    bool findRun(const std::u32string &text, const std::function<bool(char32_t)> &accepts, size_t &begin, size_t &end);

    /** Split tokens on punctuation and discard the punctuation characters. */
    std::vector<std::u32string> filterPunctuation(const std::vector<std::u32string> &tokens);

    /** Strip leading and trailing whitespace, removing empty tokens. */
    std::vector<std::u32string> stripWhitespace(const std::vector<std::u32string> &tokens);

} // namespace Tifa::Text

#endif // TIFA_TEXTUTILS_H
