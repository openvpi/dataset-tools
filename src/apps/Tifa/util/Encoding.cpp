#include "Encoding.h"

#include <algorithm>

#include "Levenshtein.h"

namespace Tifa {

    namespace {

        /** One encodable pronunciation of a word. */
        struct Candidate {
            int reading = 0;
            std::vector<int64_t> tokens;
            std::vector<int64_t> groups;
            std::vector<std::string> phonemes;
            std::vector<std::string> scripts;
        };

        /** ``qualify_symbol``; global symbols are empty at inference. */
        std::string qualifySymbol(const std::string &symbol, const std::string &defaultLanguage) {
            if (symbol.find('/') == std::string::npos && !defaultLanguage.empty()) {
                return defaultLanguage + "/" + symbol;
            }
            return symbol;
        }

        /**
         * ``resolve_phoneme`` with the inference configuration, where the
         * global and stop symbol sets are empty.
         *
         * @param symbol Receives the exact vocabulary symbol, or the qualified
         *               fallback when the phoneme stays unknown.
         */
        bool resolvePhoneme(const std::string &phoneme, const std::string &language,
                            const std::vector<std::string> &languages, const Vocabulary &vocabulary, int &token,
                            std::string &symbol) {
            std::vector<std::string> candidates;
            if (language == kAnyLanguage) {
                candidates = languages;
            } else if (!language.empty()) {
                candidates.push_back(language);
            }
            symbol = qualifySymbol(phoneme, candidates.empty() ? std::string() : candidates.front());
            return vocabulary.resolve(phoneme, candidates, token, symbol);
        }

        std::string joinList(const std::vector<std::string> &values) {
            std::string result = "[";
            for (size_t i = 0; i < values.size(); ++i) {
                result += (i == 0 ? "" : ", ") + values[i];
            }
            return result + "]";
        }

        /** ``_pack_paths``: lay out the candidates of every word in a shared grid. */
        void packPaths(const std::vector<std::vector<Candidate>> &paths, const std::vector<G2PWord> &words,
                       PathGrid &grid) {
            std::vector<std::vector<std::vector<int64_t>>> profiles;
            profiles.reserve(paths.size());
            int64_t capacity = 0;
            size_t maxCount = 0;
            for (const std::vector<Candidate> &candidates : paths) {
                std::vector<std::vector<int64_t>> profile;
                if (!candidates.empty()) {
                    std::vector<std::vector<int64_t>> raw;
                    raw.reserve(candidates.size());
                    for (const Candidate &candidate : candidates) {
                        raw.push_back(candidate.tokens);
                    }
                    for (const auto &row : Sequence::alignMultipleSequences(raw)) {
                        std::vector<int64_t> values;
                        values.reserve(row.size());
                        for (const std::optional<int64_t> &token : row) {
                            values.push_back(token.value_or(0));
                        }
                        profile.push_back(std::move(values));
                    }
                }
                capacity += profile.empty() ? 0 : static_cast<int64_t>(profile.front().size());
                maxCount = std::max(maxCount, candidates.size());
                profiles.push_back(std::move(profile));
            }

            const int64_t width = std::max<int64_t>(1, static_cast<int64_t>(maxCount));
            grid = PathGrid{};
            grid.capacity = std::max<int64_t>(1, capacity);
            grid.width = width;
            grid.wordCount = static_cast<int64_t>(paths.size());
            grid.tokens.assign(static_cast<size_t>(grid.capacity * width), 0);
            grid.groups.assign(static_cast<size_t>(grid.capacity * width), 0);
            grid.words.assign(static_cast<size_t>(grid.capacity), 0);
            grid.candidates.assign(static_cast<size_t>(grid.wordCount * width), 0);
            grid.lexicon.assign(paths.size(), {});
            grid.texts.reserve(words.size());

            int64_t offset = 0;
            for (size_t w = 0; w < paths.size(); ++w) {
                const std::vector<Candidate> &candidates = paths[w];
                const std::vector<std::vector<int64_t>> &profile = profiles[w];
                const int64_t size = profile.empty() ? 0 : static_cast<int64_t>(profile.front().size());
                const int64_t count = static_cast<int64_t>(candidates.size());
                for (int64_t c = 0; c < count; ++c) {
                    grid.candidates[static_cast<size_t>(static_cast<int64_t>(w) * width + c)] = 1;
                }
                for (int64_t p = 0; p < size; ++p) {
                    grid.words[static_cast<size_t>(offset + p)] = static_cast<int64_t>(w) + 1;
                }
                for (int64_t c = 0; c < count; ++c) {
                    for (int64_t p = 0; p < size; ++p) {
                        grid.tokens[static_cast<size_t>((offset + p) * width + c)] = profile[c][static_cast<size_t>(p)];
                    }
                }
                // Groups skip the positions the candidate leaves empty.
                for (int64_t c = 0; c < count; ++c) {
                    size_t fill = 0;
                    for (int64_t p = 0; p < size; ++p) {
                        if (profile[c][static_cast<size_t>(p)] == 0) {
                            continue;
                        }
                        if (fill < candidates[c].groups.size()) {
                            grid.groups[static_cast<size_t>((offset + p) * width + c)] = candidates[c].groups[fill];
                        }
                        fill += 1;
                    }
                }
                for (const Candidate &candidate : candidates) {
                    grid.lexicon[w].push_back(CandidateInfo{candidate.reading, candidate.phonemes, candidate.scripts});
                }
                offset += size;
            }
            for (const G2PWord &word : words) {
                grid.texts.push_back(word.text);
            }
        }

    } // namespace

    bool encodePaths(const std::vector<G2PWord> &words, const Vocabulary &vocabulary, OovHandling oovHandling,
                     const std::vector<std::string> &languages, PathGrid &grid, std::string &error) {
        std::vector<std::vector<Candidate>> paths;
        paths.reserve(words.size());

        for (const G2PWord &word : words) {
            std::vector<Candidate> candidates;
            for (size_t readingIndex = 0; readingIndex < word.readings.size(); ++readingIndex) {
                for (const G2PPath &path : word.readings[readingIndex].paths) {
                    Candidate candidate;
                    candidate.reading = static_cast<int>(readingIndex);
                    std::vector<std::string> unknown;
                    for (const G2PGroup &group : path) {
                        if (group.phonemes.empty()) {
                            error = "An empty pronunciation must be an empty path, not an empty group.";
                            return false;
                        }
                        std::vector<int64_t> groupTokens;
                        std::vector<std::string> groupPhonemes;
                        for (const std::string &phoneme : group.phonemes) {
                            int token = 0;
                            std::string symbol;
                            if (!resolvePhoneme(phoneme, word.language, languages, vocabulary, token, symbol)) {
                                unknown.push_back(phoneme);
                                continue;
                            }
                            if (token < kNumReservedTokens) {
                                error = "Reserved token in G2P output: '" + phoneme + "'";
                                return false;
                            }
                            groupTokens.push_back(token);
                            groupPhonemes.push_back(symbol);
                        }
                        if (!groupTokens.empty()) {
                            candidate.scripts.push_back(group.script);
                            candidate.tokens.insert(candidate.tokens.end(), groupTokens.begin(), groupTokens.end());
                            candidate.phonemes.insert(candidate.phonemes.end(), groupPhonemes.begin(),
                                                      groupPhonemes.end());
                            candidate.groups.insert(candidate.groups.end(), groupTokens.size(),
                                                    static_cast<int64_t>(candidate.scripts.size()));
                        }
                    }
                    if (!unknown.empty()) {
                        if (oovHandling == OovHandling::Force) {
                            continue;
                        }
                        error = "Unknown phonemes " + joinList(unknown) + " in word '" + word.text + "'";
                        return false;
                    }
                    candidates.push_back(std::move(candidate));
                }
            }
            if (candidates.empty() && oovHandling != OovHandling::Force) {
                error = "No pronunciation paths for word '" + word.text + "'";
                return false;
            }
            paths.push_back(std::move(candidates));
        }

        packPaths(paths, words, grid);
        return true;
    }

    void firstChoices(const PathGrid &grid, std::vector<int64_t> &choices) {
        choices.assign(static_cast<size_t>(grid.wordCount), 0);
        for (int64_t w = 0; w < grid.wordCount; ++w) {
            for (int64_t c = 0; c < grid.width; ++c) {
                if (grid.candidates[static_cast<size_t>(w * grid.width + c)] != 0) {
                    choices[static_cast<size_t>(w)] = c + 1;
                    break;
                }
            }
        }
    }

} // namespace Tifa
