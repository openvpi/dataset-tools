#pragma once

/// @file G2PTypes.h
/// @brief Data types for grapheme-to-phoneme conversion results.

#include <string>
#include <vector>

namespace dstools {

/// @brief Result of a single word's G2P conversion.
struct G2PResult {
    std::string word;                    ///< Input word.
    std::vector<std::string> phonemes;   ///< Converted phoneme sequence.
};

} // namespace dstools
