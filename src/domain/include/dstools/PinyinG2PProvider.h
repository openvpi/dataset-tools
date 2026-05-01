#pragma once

/// @file PinyinG2PProvider.h
/// @brief Mandarin Pinyin grapheme-to-phoneme provider.

#include <dsfw/IG2PProvider.h>

#include <string>
#include <vector>

namespace dstools {

/// @brief Converts Chinese text to Pinyin phoneme sequences,
///        implementing IG2PProvider.
class PinyinG2PProvider : public IG2PProvider {
public:
    PinyinG2PProvider() = default;
    ~PinyinG2PProvider() override = default;

    /// @brief Return the human-readable provider name.
    const char *providerName() const override;

    /// @brief Convert a text string to phoneme sequences.
    /// @param text Input Chinese text.
    /// @param langCode Language code (e.g. "zh").
    /// @return A vector of G2PResult or an error.
    Result<std::vector<G2PResult>> convert(const std::string &text,
                                            const std::string &langCode) override;

    /// @brief Convert a single word to its phoneme representation.
    /// @param word Input word.
    /// @param langCode Language code (e.g. "zh").
    /// @return A G2PResult or an error.
    Result<G2PResult> convertWord(const std::string &word,
                                   const std::string &langCode) override;

private:
    /// @brief Internal helper that converts Chinese text to Pinyin syllables.
    /// @param text Input Chinese text.
    /// @return Vector of Pinyin strings.
    static std::vector<std::string> toPinyin(const std::string &text);
};

} // namespace dstools
