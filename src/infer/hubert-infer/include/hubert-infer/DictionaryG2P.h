/// @file DictionaryG2P.h
/// @brief Dictionary-based grapheme-to-phoneme converter for HuBERT-FA.

#pragma once
#include <hubert-infer/HubertInferGlobal.h>

#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace HFA {

    /// @brief Loads a pronunciation dictionary and converts input text to phoneme/word
    ///        sequences with word-to-phoneme index mapping.
    class HUBERT_INFER_EXPORT DictionaryG2P {
    public:
        /// @brief Constructs the G2P converter from a dictionary file.
        /// @param dictionaryPath Path to the pronunciation dictionary.
        /// @param language Default language identifier.
        DictionaryG2P(const std::string &dictionaryPath, const std::string &language);

        /// @brief Converts input text to phoneme and word sequences.
        /// @param inputText Text to convert.
        /// @param language Language identifier for dictionary lookup.
        /// @return Tuple of (phoneme sequence, word sequence, phoneme-to-word index mapping).
        std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<int>>
            convert(const std::string &inputText, const std::string &language);

    private:
        std::unordered_map<std::string, std::unordered_map<std::string, std::vector<std::string>>> dictionary_; ///< Language-keyed pronunciation dictionary.
        std::string language_; ///< Default language.
    };

} // namespace HFA
