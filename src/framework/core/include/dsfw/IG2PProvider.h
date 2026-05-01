#pragma once

/// @file IG2PProvider.h
/// @brief Grapheme-to-phoneme provider interface.

#include <dsfw/G2PTypes.h>
#include <dstools/Result.h>

#include <string>
#include <vector>

namespace dstools {

/// @brief Abstract interface for G2P conversion backends (e.g. Pinyin, Romaji).
class IG2PProvider {
public:
    virtual ~IG2PProvider() = default;

    /// @brief Return the provider identifier.
    /// @return Provider name string.
    virtual const char *providerName() const = 0;

    /// @brief Convert text to phonemes.
    /// @param text Input text to convert.
    /// @param langCode Language code (e.g. "zh", "ja").
    /// @return Vector of G2P results or error.
    virtual Result<std::vector<G2PResult>> convert(const std::string &text,
                                                    const std::string &langCode) = 0;

    /// @brief Convert a single word to phonemes.
    /// @param word Input word to convert.
    /// @param langCode Language code (e.g. "zh", "ja").
    /// @return G2P result or error.
    virtual Result<G2PResult> convertWord(const std::string &word,
                                           const std::string &langCode) = 0;
};

} // namespace dstools
