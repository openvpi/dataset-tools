#ifndef TIFA_LEVENSHTEIN_H
#define TIFA_LEVENSHTEIN_H

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

namespace Tifa::Sequence {

    /**
     * Align orig to mut via Levenshtein DP.
     *
     * ``orig`` may already contain gaps, which never compare equal to a token.
     * Cost is 0 for equality and 1 for substitution and indels. Backtracking
     * prefers a match over a deletion over an insertion. Both returned
     * sequences have the same length, with nullopt marking gaps.
     */
    template <typename T>
    std::pair<std::vector<std::optional<T>>, std::vector<std::optional<T>>>
    align(const std::vector<std::optional<T>> &orig, const std::vector<T> &mut) {
        const size_t n = orig.size();
        const size_t m = mut.size();
        std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));
        for (size_t i = 0; i <= n; ++i) {
            dp[i][0] = static_cast<int>(i);
        }
        for (size_t j = 0; j <= m; ++j) {
            dp[0][j] = static_cast<int>(j);
        }

        for (size_t i = 1; i <= n; ++i) {
            for (size_t j = 1; j <= m; ++j) {
                const int cost = orig[i - 1] == mut[j - 1] ? 0 : 1;
                const int matchValue = dp[i - 1][j - 1] + cost;
                const int deleteValue = dp[i - 1][j] + 1;
                const int insertValue = dp[i][j - 1] + 1;
                if (matchValue <= deleteValue && matchValue <= insertValue) {
                    dp[i][j] = matchValue;
                } else if (deleteValue <= insertValue) {
                    dp[i][j] = deleteValue;
                } else {
                    dp[i][j] = insertValue;
                }
            }
        }

        std::vector<std::optional<T>> alignedOrig;
        std::vector<std::optional<T>> alignedMut;
        size_t i = n;
        size_t j = m;
        while (i > 0 || j > 0) {
            bool matchOk = false;
            if (i > 0 && j > 0) {
                const int cost = orig[i - 1] == mut[j - 1] ? 0 : 1;
                matchOk = dp[i][j] == dp[i - 1][j - 1] + cost;
            }
            if (matchOk) {
                alignedOrig.emplace_back(orig[i - 1]);
                alignedMut.emplace_back(mut[j - 1]);
                i -= 1;
                j -= 1;
            } else if (i > 0 && dp[i][j] == dp[i - 1][j] + 1) {
                alignedOrig.emplace_back(orig[i - 1]);
                alignedMut.emplace_back(std::nullopt);
                i -= 1;
            } else {
                alignedOrig.emplace_back(std::nullopt);
                alignedMut.emplace_back(mut[j - 1]);
                j -= 1;
            }
        }

        std::reverse(alignedOrig.begin(), alignedOrig.end());
        std::reverse(alignedMut.begin(), alignedMut.end());
        return {alignedOrig, alignedMut};
    }

    /** Column-wise consensus from a row-oriented profile. Ties keep the earliest value. */
    template <typename T>
    std::vector<std::optional<T>> consensus(const std::vector<std::vector<std::optional<T>>> &rows) {
        if (rows.empty()) {
            return {};
        }
        const size_t columns = rows.front().size();
        std::vector<std::optional<T>> result;
        result.reserve(columns);
        for (size_t column = 0; column < columns; ++column) {
            std::vector<std::pair<T, int>> counts;
            for (const auto &row : rows) {
                const std::optional<T> &value = row[column];
                if (!value.has_value()) {
                    continue;
                }
                bool found = false;
                for (auto &[symbol, count] : counts) {
                    if (symbol == *value) {
                        count += 1;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    counts.emplace_back(*value, 1);
                }
            }
            if (counts.empty()) {
                result.emplace_back(std::nullopt);
                continue;
            }
            const auto best = std::max_element(counts.begin(), counts.end(),
                                               [](const auto &a, const auto &b) { return a.second < b.second; });
            result.emplace_back(best->first);
        }
        return result;
    }

    /** Align newPath to the existing profile and merge it in. */
    template <typename T>
    std::vector<std::vector<std::optional<T>>> mergeProfile(const std::vector<std::vector<std::optional<T>>> &rows,
                                                            const std::vector<T> &newPath) {
        const std::vector<std::optional<T>> center = consensus(rows);
        const auto [alignedCenter, alignedPath] = align(center, newPath);

        std::vector<std::vector<std::optional<T>>> newRows;
        newRows.reserve(rows.size() + 1);
        for (const auto &oldRow : rows) {
            std::vector<std::optional<T>> newRow;
            newRow.reserve(alignedCenter.size());
            size_t oldIndex = 0;
            for (const std::optional<T> &token : alignedCenter) {
                if (token.has_value()) {
                    newRow.push_back(oldRow[oldIndex]);
                    oldIndex += 1;
                } else {
                    newRow.emplace_back(std::nullopt);
                }
            }
            newRows.push_back(std::move(newRow));
        }
        newRows.push_back(alignedPath);
        return newRows;
    }

    /** Align paths while preserving every source row, including duplicates. */
    template <typename T>
    std::vector<std::vector<std::optional<T>>> alignMultipleSequences(const std::vector<std::vector<T>> &paths) {
        if (paths.empty()) {
            return {};
        }
        std::vector<std::vector<std::optional<T>>> rows;
        rows.push_back(std::vector<std::optional<T>>(paths.front().begin(), paths.front().end()));
        for (size_t i = 1; i < paths.size(); ++i) {
            rows = mergeProfile(rows, paths[i]);
        }
        return rows;
    }

} // namespace Tifa::Sequence

#endif // TIFA_LEVENSHTEIN_H
