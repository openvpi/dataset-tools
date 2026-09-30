#ifndef TIFA_HOSTSCORING_H
#define TIFA_HOSTSCORING_H

#include <cstdint>
#include <string>
#include <vector>

#include "Encoding.h"
#include "Model.h"

namespace Tifa {

    /** Joint choice of every word and the conditional score of every candidate. */
    struct CandidateScores {
        /** Candidate IDs [wordCount], 1-based; zero marks a word without a pronunciation. */
        std::vector<int64_t> choices;
        /** Scores [wordCount, width]; minus infinity for candidates that do not fit. */
        std::vector<float> scores;
    };

    /**
     * Whole-word Viterbi over the fragment costs of the score graph.
     *
     * Port of ``select_scored_paths``/``_select_sample``: every word selects one
     * complete candidate, closing a segment adds its SPACE suffix exactly once,
     * and the conditional scores fix one word's candidate while optimizing all
     * remaining words. Ties are broken by the candidate order of the previous
     * layer, which keeps the choice deterministic.
     *
     * @param vocabSize Output channels of the model, used to spread the scores.
     * @return False with *error* when no candidate of some word fits the scoring
     *         template.
     */
    bool selectScoredPaths(const PathGrid &grid, const ScoreOutput &scored, int64_t vocabSize, CandidateScores &result,
                           std::string &error);

} // namespace Tifa

#endif // TIFA_HOSTSCORING_H
