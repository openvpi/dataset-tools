#include <game-infer/WordParser.h>

#include <numeric>
#include <sstream>

namespace Game
{

    std::pair<bool, std::string> validatePhones(const std::vector<std::string> &phSeq,
                                                const std::vector<float> &phDur,
                                                const std::vector<int> &phNum) {
        if (phSeq.size() != phDur.size()) {
            return {false, "Length mismatch: " + std::to_string(phSeq.size()) + " phonemes vs " +
                               std::to_string(phDur.size()) + " durations."};
        }
        const int totalPhNum = std::accumulate(phNum.begin(), phNum.end(), 0);
        if (totalPhNum != static_cast<int>(phSeq.size())) {
            return {false, "Word span mismatch: sum of ph_num is " + std::to_string(totalPhNum) + ", expected " +
                               std::to_string(phSeq.size()) + "."};
        }
        return {true, ""};
    }

    std::vector<WordInfo> parseWords(const std::vector<std::string> &phSeq, const std::vector<float> &phDur,
                                     const std::vector<int> &phNum, const std::set<std::string> &uvVocab,
                                     const UvWordCond cond, const bool mergeConsecutiveUv) {
        std::vector<WordInfo> words;
        size_t idx = 0;

        for (const int num : phNum) {
            float durSum = 0.0f;
            for (int j = 0; j < num && idx + j < phDur.size(); ++j) {
                durSum += phDur[idx + j];
            }

            bool voiced = true;
            if (!uvVocab.empty()) {
                if (cond == UvWordCond::Lead) {
                    if (idx < phSeq.size() && uvVocab.count(phSeq[idx])) {
                        voiced = false;
                    }
                } else { // UvWordCond::All
                    bool allUv = true;
                    for (int j = 0; j < num && idx + j < phSeq.size(); ++j) {
                        if (!uvVocab.count(phSeq[idx + j])) {
                            allUv = false;
                            break;
                        }
                    }
                    if (allUv)
                        voiced = false;
                }
            }

            words.push_back(WordInfo{durSum, voiced});
            idx += num;
        }

        if (mergeConsecutiveUv) {
            return mergeConsecutiveUvWords(words);
        }
        return words;
    }

    std::vector<WordInfo> mergeConsecutiveUvWords(const std::vector<WordInfo> &words) {
        if (words.empty())
            return {};

        std::vector<WordInfo> merged;
        merged.push_back(words[0]);

        for (size_t i = 1; i < words.size(); ++i) {
            if (!words[i].voiced && !merged.back().voiced) {
                merged.back().duration += words[i].duration;
            } else {
                merged.push_back(words[i]);
            }
        }
        return merged;
    }

    std::vector<float> wordDurations(const std::vector<WordInfo> &words) {
        std::vector<float> durations;
        durations.reserve(words.size());
        for (const auto &w : words) {
            durations.push_back(w.duration);
        }
        return durations;
    }

} // namespace Game
