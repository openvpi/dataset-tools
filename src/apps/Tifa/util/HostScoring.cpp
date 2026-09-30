#include "HostScoring.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <unordered_map>
#include <utility>

namespace Tifa {

    namespace {

        constexpr float kNegInfinity = -std::numeric_limits<float>::infinity();

        /** One attempted transition of the whole-word DP. */
        struct Edge {
            int64_t source = 0;
            int64_t target = 0;
            size_t sourceIndex = 0;
            size_t targetIndex = 0;
            int64_t choice = 0;
            float cost = 0.0f;
        };

        /** One layer of the forward pass. */
        struct Layer {
            /** State keys in insertion order; ``states[i]`` belongs to ``values[i]``. */
            std::vector<int64_t> states;
            std::vector<float> values;
            /** Tie-break rank of every state, matching the sorted candidate order. */
            std::unordered_map<int64_t, int64_t> ranks;
            std::unordered_map<int64_t, std::pair<int64_t, int64_t>> parents;
            std::vector<Edge> edges;
        };

        std::pair<int64_t, int64_t> decodeState(const int64_t state, const int64_t stride) {
            return {state / stride, state % stride};
        }

        /**
         * ``_advance``: walk the fragments of a complete candidate.
         *
         * @return The target state and its cost, or nothing when the candidate
         *         exceeds the capacity of one of its segments.
         */
        std::optional<std::pair<int64_t, float>> advanceState(const int64_t state, const std::vector<int64_t> &pieces,
                                                              const int64_t choice, const ScoreOutput &scored,
                                                              const int64_t stride) {
            auto [segment, used] = decodeState(state, stride);
            float score = 0.0f;
            for (const int64_t fragment : pieces) {
                const int64_t current = scored.descriptors[static_cast<size_t>(fragment) * 2 + 1];
                if (current != segment) {
                    score += scored.tailAt(segment, used);
                    segment = current;
                    used = 0;
                }
                const int64_t length = scored.lengthAt(fragment, choice);
                if (used + length > scored.capacity[static_cast<size_t>(segment)]) {
                    return std::nullopt;
                }
                score += scored.costAt(fragment, choice, used);
                used += length;
            }
            return std::make_pair(segment * stride + used, score);
        }

        /** One sample of ``_select_sample``. */
        bool selectSample(const PathGrid &grid, const ScoreOutput &scored, const int64_t vocabSize,
                          CandidateScores &result, std::string &error) {
            const int64_t wordCount = grid.wordCount;
            const int64_t width = grid.width;
            const int64_t p1 = scored.p1;
            const int64_t stride = p1;

            std::vector<std::vector<int64_t>> pieces(static_cast<size_t>(wordCount) + 1);
            for (int64_t fragment = 1; fragment < p1; ++fragment) {
                const int64_t owner = scored.descriptors[static_cast<size_t>(fragment) * 2];
                if (owner >= 1 && owner <= wordCount) {
                    pieces[static_cast<size_t>(owner)].push_back(fragment);
                }
            }

            std::vector<Layer> forward;
            forward.reserve(static_cast<size_t>(wordCount) + 1);
            forward.push_back(Layer{});
            forward.back().states.push_back(0);
            forward.back().values.push_back(0.0f);
            forward.back().ranks.emplace(0, 0);

            for (int64_t w = 0; w < wordCount; ++w) {
                const std::vector<int64_t> &wordPieces = pieces[static_cast<size_t>(w) + 1];
                std::vector<int64_t> candidates;
                for (int64_t c = 0; c < width; ++c) {
                    if (grid.candidates[static_cast<size_t>(w * width + c)] != 0) {
                        candidates.push_back(c + 1);
                    }
                }
                if (candidates.empty()) {
                    candidates.push_back(0);
                }

                const Layer &previous = forward.back();
                Layer layer;
                std::unordered_map<int64_t, size_t> index;
                std::unordered_map<int64_t, std::pair<int64_t, int64_t>> keys;

                for (size_t s = 0; s < previous.states.size(); ++s) {
                    const int64_t source = previous.states[s];
                    const float value = previous.values[s];
                    const int64_t sourceRank = previous.ranks.at(source);
                    for (const int64_t c : candidates) {
                        int64_t target = source;
                        float cost = 0.0f;
                        if (c > 0) {
                            const auto transition = advanceState(source, wordPieces, c - 1, scored, stride);
                            if (!transition) {
                                continue;
                            }
                            target = transition->first;
                            cost = transition->second;
                        }

                        const auto insertion = index.emplace(target, layer.states.size());
                        if (insertion.second) {
                            layer.states.push_back(target);
                            layer.values.push_back(value + cost);
                            keys[target] = {sourceRank, c};
                            layer.parents[target] = {source, c};
                        } else {
                            const size_t position = insertion.first->second;
                            const float total = value + cost;
                            const std::pair<int64_t, int64_t> key = {sourceRank, c};
                            if (total > layer.values[position] ||
                                (total == layer.values[position] && key < keys[target])) {
                                layer.values[position] = total;
                                keys[target] = key;
                                layer.parents[target] = {source, c};
                            }
                        }
                        layer.edges.push_back(Edge{source, target, s, index.at(target), c, cost});
                    }
                }

                if (layer.states.empty()) {
                    error = "No complete pronunciation path fits the scoring template.";
                    return false;
                }

                std::vector<int64_t> order = layer.states;
                std::stable_sort(order.begin(), order.end(),
                                 [&keys](const int64_t a, const int64_t b) { return keys[a] < keys[b]; });
                for (size_t rank = 0; rank < order.size(); ++rank) {
                    layer.ranks[order[rank]] = static_cast<int64_t>(rank);
                }
                forward.push_back(std::move(layer));
            }

            // Terminal costs close the last segment of the alignment.
            const Layer &last = forward.back();
            std::vector<float> terminal(last.states.size(), 0.0f);
            size_t bestIndex = 0;
            float bestTotal = kNegInfinity;
            int64_t bestRank = 0;
            for (size_t i = 0; i < last.states.size(); ++i) {
                const auto [segment, used] = decodeState(last.states[i], stride);
                terminal[i] = scored.tailAt(segment, used);
                const float total = last.values[i] + terminal[i];
                const int64_t rank = last.ranks.at(last.states[i]);
                if (i == 0 || total > bestTotal || (total == bestTotal && rank < bestRank)) {
                    bestTotal = total;
                    bestRank = rank;
                    bestIndex = i;
                }
            }

            size_t stateIndex = bestIndex;
            int64_t state = last.states[stateIndex];
            for (int64_t i = wordCount - 1; i >= 0; --i) {
                const auto parent = forward[static_cast<size_t>(i) + 1].parents.at(state);
                result.choices[static_cast<size_t>(i)] = parent.second;
                state = parent.first;
            }

            // The backward pass turns the transitions into conditional scores.
            std::vector<float> backward = terminal;
            for (int64_t i = wordCount - 1; i >= 0; --i) {
                const Layer &layer = forward[static_cast<size_t>(i) + 1];
                const Layer &start = forward[static_cast<size_t>(i)];
                std::vector<float> accumulated(start.states.size(), kNegInfinity);
                for (const Edge &edge : layer.edges) {
                    const float suffix = edge.cost + backward[edge.targetIndex];
                    accumulated[edge.sourceIndex] = std::max(accumulated[edge.sourceIndex], suffix);
                    if (edge.choice > 0) {
                        float &slot = result.scores[static_cast<size_t>(i * width + edge.choice - 1)];
                        slot = std::max(slot, start.values[edge.sourceIndex] + suffix);
                    }
                }
                backward = std::move(accumulated);
            }

            int64_t count = 0;
            for (const int64_t length : scored.capacity) {
                count += length;
            }
            if (count != 0) {
                const float offset = static_cast<float>(std::log(static_cast<double>(vocabSize)));
                for (int64_t i = 0; i < wordCount; ++i) {
                    for (int64_t c = 0; c < width; ++c) {
                        float &slot = result.scores[static_cast<size_t>(i * width + c)];
                        slot = slot / static_cast<float>(count) + offset;
                    }
                }
            }
            return true;
        }

    } // namespace

    bool selectScoredPaths(const PathGrid &grid, const ScoreOutput &scored, const int64_t vocabSize,
                           CandidateScores &result, std::string &error) {
        if (scored.p1 < 1 || scored.width != grid.width) {
            error = "The score fragments do not match the candidate grid";
            return false;
        }

        result.choices.assign(static_cast<size_t>(grid.wordCount), 0);
        result.scores.assign(static_cast<size_t>(grid.wordCount * grid.width), kNegInfinity);
        return selectSample(grid, scored, vocabSize, result, error);
    }

} // namespace Tifa
