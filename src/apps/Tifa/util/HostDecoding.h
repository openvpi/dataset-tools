#ifndef TIFA_HOSTDECODING_H
#define TIFA_HOSTDECODING_H

#include <cstdint>
#include <string>
#include <vector>

namespace Tifa {

    /**
     * Flat Viterbi alignment of one utterance.
     *
     * Port of ``decode_alignment_flat``/``_decode_flat_single``: every token
     * emits at least one frame or pays *skipPenalty*, skipped tokens become
     * zero-width spans anchored at the first permitted gap of their group, and
     * prefix and suffix tokens may be skipped as well.
     *
     * @param similarity  Similarity of frame and token features, row major with
     *                    *stride* values per frame; only the first *tokens*
     *                    columns are read.
     * @param groups      Group label of every token [tokens]; consecutive tokens
     *                    of one pronunciation group keep their gap.
     * @param spans       Receives (onset, offset) frame indices [tokens, 2].
     */
    bool decodeAlignment(const float *similarity, int64_t stride, int64_t frames, int64_t tokens,
                         const std::vector<int64_t> &groups, float skipPenalty, std::vector<int64_t> &spans,
                         std::string &error);

    /**
     * Mean probability the model assigns to its own tokens, in ``[0, 1]``.
     *
     * @param logits   Token classification scores [count, vocab] of the pass
     *                 that produced the aligned tokens.
     * @param tokenIds Aligned token IDs, one per row of *logits*.
     */
    float computeAgreement(const std::vector<float> &logits, int64_t vocab, const int64_t *tokenIds, int64_t count);

} // namespace Tifa

#endif // TIFA_HOSTDECODING_H
