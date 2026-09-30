#include "AlignOutput.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <utility>

#include "textgrid.hpp"

namespace Tifa {

    namespace {

        constexpr double kEpsilon = 0.001;

        double roundMilliseconds(const double value) {
            return std::round(value * 1000.0) / 1000.0;
        }

        std::string formatSeconds(const double value) {
            char buffer[32];
            std::snprintf(buffer, sizeof(buffer), "%.3f", value);
            return buffer;
        }

        /**
         * ``_preserve_skipped_spans``: place a millisecond of silence inside the
         * permitted gaps of every skipped run, then repair overlaps.
         *
         * Runs on integer milliseconds so that the repairs cannot change the
         * decoded spans, and returns one interval per input token.
         */
        std::vector<std::pair<double, double>> preserveSkippedSpans(const std::vector<double> &spans,
                                                                    const std::vector<int64_t> &groups,
                                                                    double &totalDuration) {
            const int64_t count = static_cast<int64_t>(spans.size()) / 2;
            std::vector<std::pair<int64_t, int64_t>> original(static_cast<size_t>(count));
            for (int64_t i = 0; i < count; ++i) {
                original[static_cast<size_t>(i)] = {
                    static_cast<int64_t>(std::llround(spans[static_cast<size_t>(i) * 2] * 1000.0)),
                    static_cast<int64_t>(std::llround(spans[static_cast<size_t>(i) * 2 + 1] * 1000.0))};
            }
            int64_t duration = static_cast<int64_t>(std::llround(totalDuration * 1000.0));

            std::vector<std::pair<int64_t, int64_t>> intervals;
            int64_t i = 0;
            while (i < count) {
                const int64_t left = intervals.empty() ? 0 : intervals.back().second;
                const int64_t onset = original[static_cast<size_t>(i)].first;
                const int64_t offset = original[static_cast<size_t>(i)].second;
                if (offset > onset) {
                    const int64_t repaired = std::max(onset, left);
                    intervals.emplace_back(repaired, std::max(offset, repaired + 1));
                    i += 1;
                    continue;
                }

                int64_t end = i + 1;
                while (end < count && original[static_cast<size_t>(end)].first >=
                                           original[static_cast<size_t>(end)].second) {
                    end += 1;
                }
                const int64_t right = std::max(left, end < count ? original[static_cast<size_t>(end)].first : duration);
                int64_t gapIndex = i;
                while (gapIndex <= end) {
                    if (gapIndex == 0 || gapIndex == count ||
                        groups[static_cast<size_t>(gapIndex) - 1] != groups[static_cast<size_t>(gapIndex)]) {
                        break;
                    }
                    gapIndex += 1;
                }
                const int64_t leftCount = std::min(gapIndex, end) - i;
                const int64_t rightCount = end - i - leftCount;
                for (int64_t k = 0; k < leftCount; ++k) {
                    intervals.emplace_back(left + k, left + k + 1);
                }
                const int64_t rightStart = std::max(left + leftCount, right - rightCount);
                for (int64_t k = 0; k < rightCount; ++k) {
                    intervals.emplace_back(rightStart + k, rightStart + k + 1);
                }
                i = end;
            }

            if (!intervals.empty()) {
                duration = std::max(duration, intervals.back().second);
            }
            totalDuration = static_cast<double>(duration) / 1000.0;

            std::vector<std::pair<double, double>> result;
            result.reserve(intervals.size());
            for (const auto &[onset, offset] : intervals) {
                result.emplace_back(static_cast<double>(onset) / 1000.0, static_cast<double>(offset) / 1000.0);
            }
            return result;
        }

        std::string labelAt(const std::vector<std::string> &labels, const int64_t index) {
            if (index < 1 || index > static_cast<int64_t>(labels.size())) {
                return {};
            }
            return labels[static_cast<size_t>(index) - 1];
        }

    } // namespace

    TextGridResult buildTextGrid(const Alignment &alignment, const SkipHandling skipHandling,
                                 const std::string &phonePrefix) {
        TextGridResult result;

        std::vector<std::string> phonemes = alignment.phonemes;
        if (!phonePrefix.empty()) {
            const std::string prefix = phonePrefix + "/";
            for (std::string &phoneme : phonemes) {
                if (phoneme.rfind(prefix, 0) == 0) {
                    phoneme = phoneme.substr(prefix.size());
                }
            }
        }

        const int64_t count = alignment.tokenCount();
        if (count == 0) {
            return result;
        }
        if (static_cast<int64_t>(phonemes.size()) != count ||
            static_cast<int64_t>(alignment.groups.size()) != count ||
            static_cast<int64_t>(alignment.words.size()) != count) {
            result.report = "The alignment is inconsistent.";
            return result;
        }

        std::vector<double> spans(static_cast<size_t>(count) * 2);
        for (int64_t i = 0; i < count; ++i) {
            spans[static_cast<size_t>(i) * 2] =
                roundMilliseconds(static_cast<double>(alignment.spans[static_cast<size_t>(i) * 2]) * alignment.timestep);
            spans[static_cast<size_t>(i) * 2 + 1] = roundMilliseconds(
                static_cast<double>(alignment.spans[static_cast<size_t>(i) * 2 + 1]) * alignment.timestep);
        }
        double totalDuration = roundMilliseconds(static_cast<double>(alignment.frames) * alignment.timestep);

        // Report the tokens the decoder skipped, then handle them.
        std::vector<int64_t> skipped;
        for (int64_t i = 0; i < count; ++i) {
            if (spans[static_cast<size_t>(i) * 2] >= spans[static_cast<size_t>(i) * 2 + 1]) {
                skipped.push_back(i);
            }
        }
        for (const int64_t i : skipped) {
            result.report += "Skipped token " + std::to_string(i) + " (" + phonemes[static_cast<size_t>(i)] + ") [" +
                             formatSeconds(spans[static_cast<size_t>(i) * 2]) + ", " +
                             formatSeconds(spans[static_cast<size_t>(i) * 2 + 1]) + "]\n";
        }

        std::vector<std::string> labels = phonemes;
        std::vector<int64_t> groups = alignment.groups;
        std::vector<int64_t> owners = alignment.words;
        int64_t kept = count;

        if (!skipped.empty()) {
            if (skipHandling == SkipHandling::Discard) {
                return result;
            }
            if (skipHandling == SkipHandling::Omit) {
                std::vector<uint8_t> keep(static_cast<size_t>(count), 1);
                for (const int64_t i : skipped) {
                    keep[static_cast<size_t>(i)] = 0;
                }
                std::vector<double> keptSpans;
                std::vector<std::string> keptLabels;
                std::vector<int64_t> keptGroups;
                std::vector<int64_t> keptOwners;
                for (int64_t i = 0; i < count; ++i) {
                    if (keep[static_cast<size_t>(i)] == 0) {
                        continue;
                    }
                    keptSpans.push_back(spans[static_cast<size_t>(i) * 2]);
                    keptSpans.push_back(spans[static_cast<size_t>(i) * 2 + 1]);
                    keptLabels.push_back(labels[static_cast<size_t>(i)]);
                    keptGroups.push_back(groups[static_cast<size_t>(i)]);
                    keptOwners.push_back(owners[static_cast<size_t>(i)]);
                }
                if (keptSpans.empty()) {
                    return result;
                }
                spans = std::move(keptSpans);
                labels = std::move(keptLabels);
                groups = std::move(keptGroups);
                owners = std::move(keptOwners);
                kept = static_cast<int64_t>(labels.size());
            } else {
                const std::vector<std::pair<double, double>> repaired =
                    preserveSkippedSpans(spans, groups, totalDuration);
                spans.clear();
                for (const auto &[onset, offset] : repaired) {
                    spans.push_back(onset);
                    spans.push_back(offset);
                }
            }
        } else if (skipHandling == SkipHandling::Preserve) {
            const std::vector<std::pair<double, double>> repaired = preserveSkippedSpans(spans, groups, totalDuration);
            spans.clear();
            for (const auto &[onset, offset] : repaired) {
                spans.push_back(onset);
                spans.push_back(offset);
            }
        }

        // Phone intervals: one per token, monotone and never empty.
        struct Phone {
            double onset = 0.0;
            double offset = 0.0;
            std::string label;
        };
        std::vector<Phone> phones;
        phones.reserve(static_cast<size_t>(kept));
        for (int64_t i = 0; i < kept; ++i) {
            double onset = spans[static_cast<size_t>(i) * 2];
            double offset = spans[static_cast<size_t>(i) * 2 + 1];
            if (!phones.empty() && onset < phones.back().offset) {
                onset = phones.back().offset;
            }
            if (offset <= onset) {
                offset = onset + kEpsilon;
            }
            phones.push_back(Phone{onset, offset, labels[static_cast<size_t>(i)]});
        }
        if (!phones.empty() && phones.back().offset > totalDuration) {
            totalDuration = phones.back().offset;
        }

        textgrid::TextGrid grid(0.0, totalDuration);

        // Semantic words and pronunciation groups aggregate consecutive tokens.
        const std::pair<const char *, const std::vector<int64_t> *> tiers[2] = {
            {"texts", &owners},
            {"words", &groups},
        };
        for (const auto &[name, tierOwners] : tiers) {
            const std::vector<std::string> &tierLabels = name == std::string("texts") ? alignment.texts : alignment.scripts;
            auto tier = std::make_shared<textgrid::IntervalTier>(name, 0.0, totalDuration);
            std::vector<std::pair<double, double>> intervals;
            int64_t i = 0;
            while (i < kept) {
                const int64_t owner = (*tierOwners)[static_cast<size_t>(i)];
                int64_t j = i + 1;
                while (j < kept && (*tierOwners)[static_cast<size_t>(j)] == owner) {
                    j += 1;
                }
                double onset = phones[static_cast<size_t>(i)].onset;
                double offset = phones[static_cast<size_t>(j - 1)].offset;
                if (!intervals.empty() && onset < intervals.back().second) {
                    onset = intervals.back().second;
                }
                if (offset <= onset) {
                    offset = onset + kEpsilon;
                }
                intervals.emplace_back(onset, offset);
                tier->AppendInterval(textgrid::Interval(onset, offset, labelAt(tierLabels, owner)));
                i = j;
            }
            grid.AppendTier(tier);
        }

        auto phoneTier = std::make_shared<textgrid::IntervalTier>("phones", 0.0, totalDuration);
        for (const Phone &phone : phones) {
            phoneTier->AppendInterval(textgrid::Interval(phone.onset, phone.offset, phone.label));
        }
        grid.AppendTier(phoneTier);

        std::ostringstream stream;
        stream << grid;
        result.content = stream.str();
        return result;
    }

} // namespace Tifa
