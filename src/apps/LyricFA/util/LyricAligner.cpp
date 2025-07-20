#include "LyricAligner.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace LyricFA {

    int LyricAligner::longestCommonSubsequence(const std::vector<std::string> &seq1,
                                               const std::vector<std::string> &seq2) {
        const int len1 = seq1.size();
        const int len2 = seq2.size();
        std::vector<std::vector<int>> dp(len1 + 1, std::vector<int>(len2 + 1, 0));

        for (int i = 1; i <= len1; ++i) {
            for (int j = 1; j <= len2; ++j) {
                if (seq1[i - 1] == seq2[j - 1]) {
                    dp[i][j] = dp[i - 1][j - 1] + 1;
                } else {
                    dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
                }
            }
        }
        return dp[len1][len2];
    }

    AlignmentResult LyricAligner::findBestMatchWindow(const std::vector<std::string> &referenceTextTokens,
                                                      const std::vector<std::string> &referencePronunciation,
                                                      const std::vector<std::string> &searchPronunciation,
                                                      const std::vector<std::string> &searchText) {
        int maxMatchLength = 0;
        int bestStartIdx = -1;
        const int n = referencePronunciation.size();
        const int m = searchPronunciation.size();

        if (n < m) {
            return {0, std::min(n, m), {}, {}};
        }

        for (int startIdx = 0; startIdx <= n - m; ++startIdx) {
            std::vector<std::string> window;
            if (startIdx + m <= n) {
                window = std::vector<std::string>(referencePronunciation.begin() + startIdx,
                                                  referencePronunciation.begin() + startIdx + m);
            } else {
                window =
                    std::vector<std::string>(referencePronunciation.begin() + startIdx, referencePronunciation.end());
            }

            const int currentMatchLength = longestCommonSubsequence(searchPronunciation, window);
            if (currentMatchLength >= maxMatchLength) {
                maxMatchLength = currentMatchLength;
                bestStartIdx = startIdx;
            }
        }

        if (bestStartIdx == -1) {
            bestStartIdx = 0;
            maxMatchLength = std::min(n, m);
        } else {
            maxMatchLength = std::min(n - bestStartIdx, m);
            if (maxMatchLength < m) {
                bestStartIdx = std::max(0, bestStartIdx - (m - maxMatchLength));
                maxMatchLength = std::min(n - bestStartIdx, m);
            }
        }

        std::vector<std::string> textDiffs;
        std::vector<std::string> pronunciationDiffs;

        for (int position = 0; position < maxMatchLength; ++position) {
            if (position < maxMatchLength &&
                referencePronunciation[bestStartIdx + position] != searchPronunciation[position]) {
                textDiffs.push_back("(" + searchText[position] + "->" + referenceTextTokens[bestStartIdx + position] +
                                    ", " + std::to_string(position) + ")");
                pronunciationDiffs.push_back("(" + searchPronunciation[position] + "->" +
                                             referencePronunciation[bestStartIdx + position] + ", " +
                                             std::to_string(position) + ")");
            }
        }

        return {bestStartIdx, bestStartIdx + maxMatchLength, textDiffs, pronunciationDiffs};
    }

    std::tuple<std::vector<std::string>, std::vector<std::string>, std::string, std::string>
        LyricAligner::alignSequences(const std::vector<std::string> &searchText,
                                     const std::vector<std::string> &searchPronunciation,
                                     const std::vector<std::string> &referencePronunciation,
                                     const std::vector<std::string> &referenceText, const bool showDeletions,
                                     const bool showInsertions, const bool showSubstitutions) {

        std::vector<std::string> refText = referenceText.empty() ? referencePronunciation : referenceText;

        if (searchPronunciation.size() > referencePronunciation.size()) {
            throw std::invalid_argument("Search pronunciation longer than reference");
        }
        if (refText.size() != referencePronunciation.size()) {
            throw std::invalid_argument("Reference text and pronunciation size mismatch");
        }

        const auto [startIdx, endIdx, textChanges, pronunciationChanges] =
            findBestMatchWindow(refText, referencePronunciation, searchPronunciation, searchText);

        const std::vector<std::string> matchedPronunciation(referencePronunciation.begin() + startIdx,
                                                            referencePronunciation.begin() + endIdx);

        if (!matchedPronunciation.empty() &&
            (matchedPronunciation.front() == searchPronunciation.front() ||
             matchedPronunciation.back() == searchPronunciation.back()) &&
            textChanges.size() <= 1) {

            const std::vector<std::string> matchedText(refText.begin() + startIdx, refText.begin() + endIdx);

            return {matchedText, matchedPronunciation, join(textChanges, " "), join(pronunciationChanges, " ")};
        }

        std::vector<AlignmentDetails> alignmentCandidates;
        const int windowSize = searchPronunciation.size();

        const int maxWindow = std::min(windowSize + 10, static_cast<int>(referencePronunciation.size()));
        for (int window = windowSize; window <= maxWindow; ++window) {
            const int startRange = std::max(0, startIdx - 10);
            const int endRange = std::min(static_cast<int>(referencePronunciation.size()) - window, endIdx + 10);

            for (int start_idx = startRange; start_idx <= endRange; ++start_idx) {
                if (start_idx + window > referencePronunciation.size())
                    continue;

                std::vector<std::string> windowPronunciation(referencePronunciation.begin() + start_idx,
                                                             referencePronunciation.begin() + start_idx + window);

                std::vector<std::string> windowText(refText.begin() + start_idx, refText.begin() + start_idx + window);

                alignmentCandidates.push_back(
                    computeAlignmentDetails(windowText, windowPronunciation, searchPronunciation, searchText));
            }
        }

        if (alignmentCandidates.empty()) {
            return {{}, {}, "", ""};
        }

        const auto bestIt = std::min_element(
            alignmentCandidates.begin(), alignmentCandidates.end(),
            [](const AlignmentDetails &a, const AlignmentDetails &b) { return a.editDistance < b.editDistance; });

        const AlignmentDetails &bestAlignment = *bestIt;

        return {
            bestAlignment.alignedText, bestAlignment.alignedPronunciation,
            formatOperations(bestAlignment.textOperations, showDeletions, showInsertions, showSubstitutions),
            formatOperations(bestAlignment.pronunciationOperations, showDeletions, showInsertions, showSubstitutions)};
    }

    std::string LyricAligner::join(const std::vector<std::string> &tokens, const std::string &delimiter) {
        if (tokens.empty())
            return "";

        std::ostringstream oss;
        std::copy(tokens.begin(), tokens.end() - 1, std::ostream_iterator<std::string>(oss, delimiter.c_str()));
        oss << tokens.back();
        return oss.str();
    }

    std::string LyricAligner::formatOperations(const std::vector<Step> &operations, const bool showDel,
                                               const bool showIns, const bool showSub) {
        std::vector<std::string> formattedOps;
        for (size_t position = 0; position < operations.size(); ++position) {
            const auto &[original, modified] = operations[position];
            if (original != modified) {
                if (!original.empty() && modified.empty() && showDel) {
                    formattedOps.push_back("(" + original + "->, " + std::to_string(position) + ")");
                } else if (original.empty() && !modified.empty() && showIns) {
                    formattedOps.push_back("(->" + modified + ", " + std::to_string(position) + ")");
                } else if (!original.empty() && !modified.empty() && showSub) {
                    formattedOps.push_back("(" + modified + "->" + original + ", " + std::to_string(position) + ")");
                }
            }
        }
        return join(formattedOps, " ");
    }

    std::vector<std::vector<int>> LyricAligner::createDpTable(const int rows, const int cols, const int deletionCost,
                                                              const int insertionCost) {
        std::vector<std::vector<int>> dp(rows + 1, std::vector<int>(cols + 1, 0));

        for (int i = 0; i <= rows; ++i) {
            dp[i][0] = i * deletionCost;
        }

        for (int j = 0; j <= cols; ++j) {
            dp[0][j] = j * insertionCost;
        }

        return dp;
    }

    int LyricAligner::fillDpTable(std::vector<std::vector<int>> &dp, const std::vector<std::string> &sourceSeq,
                                  const std::vector<std::string> &targetSeq, const int deletionCost,
                                  const int insertionCost, const int substitutionCost) {
        const int rows = sourceSeq.size();
        const int cols = targetSeq.size();

        for (int i = 1; i <= rows; ++i) {
            for (int j = 1; j <= cols; ++j) {
                if (sourceSeq[i - 1] == targetSeq[j - 1]) {
                    dp[i][j] = dp[i - 1][j - 1];
                } else {
                    dp[i][j] = std::min({dp[i - 1][j - 1] + substitutionCost, dp[i][j - 1] + insertionCost,
                                         dp[i - 1][j] + deletionCost});
                }
            }
        }
        return dp[rows][cols];
    }

    std::pair<std::vector<Step>, std::vector<Step>> LyricAligner::traceAlignmentPath(
        const std::vector<std::vector<int>> &dp, const std::vector<std::string> &referenceTextTokens,
        const std::vector<std::string> &sourcePronunciation, const std::vector<std::string> &targetPronunciation,
        const std::vector<std::string> &searchText, const int deletionCost, const int insertionCost,
        const int substitutionCost) {

        std::vector<Step> textOperations;
        std::vector<Step> pronunciationOperations;

        int i = sourcePronunciation.size();
        int j = targetPronunciation.size();

        while (i > 0 || j > 0) {
            if (i > 0 && j > 0 && sourcePronunciation[i - 1] == targetPronunciation[j - 1]) {
                pronunciationOperations.insert(pronunciationOperations.begin(),
                                               {targetPronunciation[j - 1], targetPronunciation[j - 1]});
                textOperations.insert(textOperations.begin(), {referenceTextTokens[i - 1], referenceTextTokens[i - 1]});
                i--;
                j--;
            } else {
                if (i > 0 && j > 0 && dp[i][j] == dp[i - 1][j - 1] + substitutionCost) {
                    pronunciationOperations.insert(pronunciationOperations.begin(),
                                                   {sourcePronunciation[i - 1], targetPronunciation[j - 1]});
                    textOperations.insert(textOperations.begin(), {referenceTextTokens[i - 1], searchText[j - 1]});
                    i--;
                    j--;
                } else if (j > 0 && dp[i][j] == dp[i][j - 1] + insertionCost) {
                    pronunciationOperations.insert(pronunciationOperations.begin(), {"", targetPronunciation[j - 1]});
                    textOperations.insert(textOperations.begin(), {"", searchText[j - 1]});
                    j--;
                } else if (i > 0 && dp[i][j] == dp[i - 1][j] + deletionCost) {
                    pronunciationOperations.insert(pronunciationOperations.begin(), {sourcePronunciation[i - 1], ""});
                    textOperations.insert(textOperations.begin(), {referenceTextTokens[i - 1], ""});
                    i--;
                }
            }
        }

        return {textOperations, pronunciationOperations};
    }

    AlignmentDetails LyricAligner::computeAlignmentDetails(const std::vector<std::string> &referenceTextTokens,
                                                           const std::vector<std::string> &sourcePronunciation,
                                                           const std::vector<std::string> &targetPronunciation,
                                                           const std::vector<std::string> &searchText,
                                                           const int deletionCost, const int insertionCost,
                                                           const int substitutionCost) {

        const int rows = sourcePronunciation.size();
        const int cols = targetPronunciation.size();

        auto dp = createDpTable(rows, cols, deletionCost, insertionCost);
        const int editDistance =
            fillDpTable(dp, sourcePronunciation, targetPronunciation, deletionCost, insertionCost, substitutionCost);

        auto [textOps, pronOps] = traceAlignmentPath(dp, referenceTextTokens, sourcePronunciation, targetPronunciation,
                                                     searchText, deletionCost, insertionCost, substitutionCost);

        std::vector<std::string> alignedText;
        std::vector<std::string> alignedPronunciation;

        for (size_t idx = 0; idx < pronOps.size(); ++idx) {
            const auto &pOp = pronOps[idx];
            const auto &tOp = textOps[idx];

            if (pOp.original == pOp.modified) {
                // 匹配操作：添加参考文本和参考发音
                alignedText.push_back(tOp.original);
                alignedPronunciation.push_back(pOp.original);
            } else if (pOp.original.empty() && !pOp.modified.empty()) {
                // 插入操作：添加目标文本和目标发音
                alignedText.push_back(tOp.modified);
                alignedPronunciation.push_back(pOp.modified);
            } else if (!pOp.original.empty() && pOp.modified.empty()) {
                // 删除操作：跳过，不添加
                // 什么也不做
            } else if (!pOp.original.empty() && !pOp.modified.empty()) {
                // 替换操作：添加参考文本和参考发音
                alignedText.push_back(tOp.original);
                alignedPronunciation.push_back(pOp.original);
            }
        }

        return {editDistance, alignedText, alignedPronunciation, textOps, pronOps};
    }
}