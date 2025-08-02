#include "DictionaryG2P.h"

#include <fstream>
#include <iostream>
#include <sstream>

namespace HFA {

    DictionaryG2P::DictionaryG2P(const std::string &dictionaryPath, const std::string &language) : language_(language) {

        std::ifstream file(dictionaryPath);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open dictionary file: " + dictionaryPath);
        }

        if (dictionary_.find(language) == dictionary_.end()) {
            dictionary_[language] = new std::unordered_map<std::string, std::vector<std::string>>();
        }

        const auto &dict = dictionary_[language];

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty())
                continue;

            size_t tabPos = line.find('\t');
            if (tabPos == std::string::npos) {
                std::cerr << "Warning: Invalid dictionary line format: " << line << std::endl;
                continue;
            }

            std::string word = line.substr(0, tabPos);
            std::string phonemesStr = line.substr(tabPos + 1);

            std::istringstream iss(phonemesStr);
            std::vector<std::string> phonemes;
            std::string phone;
            while (iss >> phone) {
                phonemes.push_back(phone);
            }

            (*dict)[word] = phonemes;
        }
    }

    std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<int>>
        DictionaryG2P::convert(const std::string &inputText, const std::string &language) {
        std::vector<std::string> wordSeqRaw;
        std::istringstream iss(inputText);
        std::string word;
        while (iss >> word) {
            wordSeqRaw.push_back(word);
        }

        std::vector<std::string> phSeq = {"SP"};
        std::vector<std::string> wordSeq;
        std::vector<int> phIdxToWordIdx = {-1};
        int wordSeqIdx = 0;

        std::unordered_map<std::string, std::vector<std::string>> *dictionary;
        if (dictionary_.find(language) != dictionary_.end()) {
            dictionary = dictionary_.find(language)->second;
        } else {
            std::cerr << "Error: Language '" << language << "' not found in dictionary." << std::endl;
            return std::make_tuple(phSeq, wordSeq, phIdxToWordIdx);
        }

        for (const auto &_word : wordSeqRaw) {
            auto it = dictionary->find(_word);
            if (it == dictionary->end()) {
                std::cerr << "Warning: Word '" << _word << "' is not in the dictionary. Ignored." << std::endl;
                continue;
            }

            const auto &phonemes = it->second;
            wordSeq.push_back(_word);

            const size_t startIdx = phSeq.size();
            for (size_t i = 0; i < phonemes.size(); ++i) {
                if ((i == 0 || i == phonemes.size() - 1) && phonemes[i] == "SP") {
                    std::cerr << "Warning: The first or last phoneme of word '" << _word
                              << "' is SP, which is not allowed." << std::endl;
                    continue;
                }
                phSeq.push_back(phonemes[i]);
                phIdxToWordIdx.push_back(wordSeqIdx);
            }

            if (phSeq.size() > startIdx && phSeq.back() != "SP") {
                phSeq.push_back("SP");
                phIdxToWordIdx.push_back(-1);
            }

            ++wordSeqIdx;
        }

        return std::make_tuple(phSeq, wordSeq, phIdxToWordIdx);
    }

} // namespace HFA