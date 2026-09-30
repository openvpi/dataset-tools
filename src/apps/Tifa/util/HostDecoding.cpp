#include "HostDecoding.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Tifa {

    namespace {

        constexpr float kNegInfinity = -std::numeric_limits<float>::infinity();

        /** ``_canonicalize_skipped_spans``: anchor every skipped run inside a permitted gap. */
        void canonicalizeSkippedSpans(std::vector<int64_t> &spans, const int64_t tokens, const int64_t frames,
                                      const std::vector<uint8_t> &gapAllowed) {
            int64_t lo = 0;
            while (lo < tokens) {
                if (spans[static_cast<size_t>(lo) * 2] != spans[static_cast<size_t>(lo) * 2 + 1]) {
                    lo += 1;
                    continue;
                }
                int64_t hi = lo;
                while (hi + 1 < tokens &&
                       spans[static_cast<size_t>(hi + 1) * 2] == spans[static_cast<size_t>(hi + 1) * 2 + 1]) {
                    hi += 1;
                }
                const int64_t left = lo > 0 ? spans[static_cast<size_t>(lo - 1) * 2 + 1] : 0;
                const int64_t right = hi + 1 < tokens ? spans[static_cast<size_t>(hi + 1) * 2] : frames;
                int64_t gapIndex = lo;
                while (gapIndex <= hi + 1 && gapAllowed[static_cast<size_t>(gapIndex)] == 0) {
                    gapIndex += 1;
                }
                for (int64_t i = lo; i <= hi; ++i) {
                    const int64_t anchor = i < gapIndex ? left : right;
                    spans[static_cast<size_t>(i) * 2] = anchor;
                    spans[static_cast<size_t>(i) * 2 + 1] = anchor;
                }
                lo = hi + 1;
            }
        }

    } // namespace

    bool decodeAlignment(const float *similarity, const int64_t stride, const int64_t frames, const int64_t tokens,
                         const std::vector<int64_t> &groups, const float skipPenalty, std::vector<int64_t> &spans,
                         std::string &error) {
        if (frames < 0 || tokens < 0 || stride < tokens) {
            error = "The alignment dimensions are invalid";
            return false;
        }
        if (tokens > 0 && similarity == nullptr) {
            error = "The similarity matrix is missing";
            return false;
        }
        if (static_cast<int64_t>(groups.size()) < tokens) {
            error = "The alignment needs one group label per token";
            return false;
        }
        if (tokens == 0) {
            spans.clear();
            return true;
        }

        // Tokens of one pronunciation group keep their gap; only a group change
        // may be crossed without consuming a frame.
        std::vector<uint8_t> gapAllowed(static_cast<size_t>(tokens) + 1, 1);
        for (int64_t i = 1; i < tokens; ++i) {
            gapAllowed[static_cast<size_t>(i)] =
                groups[static_cast<size_t>(i - 1)] != groups[static_cast<size_t>(i)] ? 1 : 0;
        }

        const float penalty = skipPenalty;
        const size_t gapWidth = static_cast<size_t>(tokens) + 1;
        std::vector<float> gap(gapWidth, kNegInfinity);
        std::vector<float> token(static_cast<size_t>(tokens), kNegInfinity);
        std::vector<int8_t> gapBack(static_cast<size_t>(frames + 1) * gapWidth, 0);
        std::vector<int8_t> tokenBack(static_cast<size_t>(frames + 1) * static_cast<size_t>(tokens), 0);

        gap[0] = 0.0f;
        for (int64_t i = 0; i < tokens; ++i) {
            gap[static_cast<size_t>(i) + 1] = gap[static_cast<size_t>(i)] - penalty;
            gapBack[static_cast<size_t>(i) + 1] = 2; // Skip the token.
        }

        std::vector<float> nextToken(static_cast<size_t>(tokens), 0.0f);
        std::vector<float> nextGap(gapWidth, kNegInfinity);
        for (int64_t t = 1; t <= frames; ++t) {
            const float *row = similarity + static_cast<size_t>(t - 1) * static_cast<size_t>(stride);
            for (int64_t i = 0; i < tokens; ++i) {
                float best = token[static_cast<size_t>(i)];
                if (gap[static_cast<size_t>(i)] > best) {
                    best = gap[static_cast<size_t>(i)];
                    tokenBack[static_cast<size_t>(t) * static_cast<size_t>(tokens) + static_cast<size_t>(i)] =
                        1; // Enter from the gap state.
                }
                nextToken[static_cast<size_t>(i)] = best + row[static_cast<size_t>(i)];
            }
            for (int64_t i = 0; i <= tokens; ++i) {
                nextGap[static_cast<size_t>(i)] =
                    gapAllowed[static_cast<size_t>(i)] != 0 ? gap[static_cast<size_t>(i)] : kNegInfinity;
            }
            for (int64_t i = 0; i < tokens; ++i) {
                const size_t target = static_cast<size_t>(i) + 1;
                if (nextToken[static_cast<size_t>(i)] > nextGap[target]) {
                    nextGap[target] = nextToken[static_cast<size_t>(i)];
                    gapBack[static_cast<size_t>(t) * gapWidth + target] = 1; // Exit without consuming a frame.
                }
                const float skipped = nextGap[static_cast<size_t>(i)] - penalty;
                if (skipped > nextGap[target]) {
                    nextGap[target] = skipped;
                    gapBack[static_cast<size_t>(t) * gapWidth + target] = 2; // Skipped token.
                }
            }
            gap.swap(nextGap);
            token.swap(nextToken);
        }

        spans.assign(static_cast<size_t>(tokens) * 2, -1);
        int64_t t = frames;
        int64_t i = tokens;
        bool inToken = false;
        while (t > 0 || i > 0 || inToken) {
            if (inToken) {
                if (spans[static_cast<size_t>(i) * 2 + 1] < 0) {
                    spans[static_cast<size_t>(i) * 2 + 1] = t;
                }
                spans[static_cast<size_t>(i) * 2] = t - 1;
                const int8_t entered =
                    tokenBack[static_cast<size_t>(t) * static_cast<size_t>(tokens) + static_cast<size_t>(i)];
                t -= 1;
                if (entered == 1) {
                    inToken = false;
                }
            } else {
                const int8_t source = gapBack[static_cast<size_t>(t) * gapWidth + static_cast<size_t>(i)];
                if (source == 0) {
                    t -= 1;
                } else if (source == 1) {
                    i -= 1;
                    inToken = true;
                } else {
                    i -= 1;
                    spans[static_cast<size_t>(i) * 2] = t;
                    spans[static_cast<size_t>(i) * 2 + 1] = t;
                }
            }
        }

        canonicalizeSkippedSpans(spans, tokens, frames, gapAllowed);
        return true;
    }

    float computeAgreement(const std::vector<float> &logits, const int64_t vocab, const int64_t *tokenIds,
                           const int64_t count) {
        if (count <= 0 || vocab <= 0 || static_cast<int64_t>(logits.size()) < count * vocab) {
            return 0.0f;
        }

        double total = 0.0;
        for (int64_t i = 0; i < count; ++i) {
            const float *row = logits.data() + static_cast<size_t>(i) * static_cast<size_t>(vocab);
            float maximum = row[0];
            for (int64_t v = 1; v < vocab; ++v) {
                maximum = std::max(maximum, row[static_cast<size_t>(v)]);
            }
            double sum = 0.0;
            for (int64_t v = 0; v < vocab; ++v) {
                sum += std::exp(static_cast<double>(row[static_cast<size_t>(v)] - maximum));
            }
            const int64_t token = tokenIds[i];
            if (token < 0 || token >= vocab) {
                continue;
            }
            total += std::exp(static_cast<double>(row[static_cast<size_t>(token)] - maximum)) / sum;
        }
        return static_cast<float>(total / static_cast<double>(count));
    }

} // namespace Tifa
