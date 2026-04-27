#pragma once

#include <set>
#include <string>
#include <vector>

#include "GameGlobal.h"

namespace Game
{

    enum class UvWordCond {
        Lead, // Leading phoneme determines unvoiced
        All   // All phonemes must be unvoiced
    };

    enum class UvNoteCond {
        Predict, // Model predicts note v/uv
        Follow   // Note v/uv follows word v/uv
    };

    struct WordInfo {
        float duration; // Word duration in seconds
        bool voiced;    // true = voiced, false = unvoiced
    };

    /**
     * Validate phoneme sequence consistency.
     * @return {is_valid, error_message}
     */
    GAME_INFER_EXPORT std::pair<bool, std::string> validatePhones(const std::vector<std::string> &phSeq,
                                                                   const std::vector<float> &phDur,
                                                                   const std::vector<int> &phNum);

    /**
     * Convert phoneme sequence to word durations and v/uv flags.
     * Port of Python inference/utils.py parse_words().
     */
    GAME_INFER_EXPORT std::vector<WordInfo> parseWords(const std::vector<std::string> &phSeq,
                                                       const std::vector<float> &phDur,
                                                       const std::vector<int> &phNum,
                                                       const std::set<std::string> &uvVocab = {},
                                                       UvWordCond cond = UvWordCond::Lead,
                                                       bool mergeConsecutiveUv = false);

    /**
     * Merge consecutive unvoiced words into one.
     * Port of Python inference/utils.py merge_consecutive_uv_words().
     */
    GAME_INFER_EXPORT std::vector<WordInfo> mergeConsecutiveUvWords(const std::vector<WordInfo> &words);

    /**
     * Extract word durations only (for use as known_durations in align mode).
     */
    GAME_INFER_EXPORT std::vector<float> wordDurations(const std::vector<WordInfo> &words);

} // namespace Game
