#ifndef LYRICALIGNER_H
#define LYRICALIGNER_H

#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace LyricFA {
    struct AlignmentResult {
        int startIdx;
        int endIdx;
        std::vector<std::string> textChanges;
        std::vector<std::string> pronunciationChanges;
    };

    struct Step {
        std::string original;
        std::string modified;
    };

    struct AlignmentDetails {
        int editDistance;
        std::vector<std::string> alignedText;
        std::vector<std::string> alignedPronunciation;
        std::vector<Step> textOperations;
        std::vector<Step> pronunciationOperations;
    };

    class LyricAligner {
    public:
        LyricAligner() = default;

        static int longestCommonSubsequence(const std::vector<std::string> &seq1, const std::vector<std::string> &seq2);
        static AlignmentResult findBestMatchWindow(const std::vector<std::string> &referenceTextTokens,
                                                   const std::vector<std::string> &referencePronunciation,
                                                   const std::vector<std::string> &searchPronunciation,
                                                   const std::vector<std::string> &searchText);

        static std::tuple<std::vector<std::string>, std::vector<std::string>, std::string, std::string> alignSequences(
            const std::vector<std::string> &searchText, const std::vector<std::string> &searchPronunciation,
            const std::vector<std::string> &referencePronunciation, const std::vector<std::string> &referenceText = {},
            bool showDeletions = false, bool showInsertions = false, bool showSubstitutions = false);

        static std::string join(const std::vector<std::string> &tokens, const std::string &delimiter);

    private:
        static std::string formatOperations(const std::vector<Step> &operations, bool showDel, bool showIns,
                                            bool showSub);

        static std::vector<std::vector<int>> createDpTable(int rows, int cols, int deletionCost, int insertionCost);

        static int fillDpTable(std::vector<std::vector<int>> &dp, const std::vector<std::string> &sourceSeq,
                               const std::vector<std::string> &targetSeq, int deletionCost, int insertionCost,
                               int substitutionCost);

        static std::pair<std::vector<Step>, std::vector<Step>> traceAlignmentPath(
            const std::vector<std::vector<int>> &dp, const std::vector<std::string> &referenceTextTokens,
            const std::vector<std::string> &sourcePronunciation, const std::vector<std::string> &targetPronunciation,
            const std::vector<std::string> &searchText);

        static AlignmentDetails computeAlignmentDetails(const std::vector<std::string> &referenceTextTokens,
                                                        const std::vector<std::string> &sourcePronunciation,
                                                        const std::vector<std::string> &targetPronunciation,
                                                        const std::vector<std::string> &searchText,
                                                        int deletionCost = 1, int insertionCost = 3,
                                                        int substitutionCost = 3);
    };

} // LyricFA

#endif // LYRICALIGNER_H
