#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <dstools/TimePos.h>
#include <type_traits>
#include <vector>

namespace dsfw::signal {

    template <typename T, int64_t DefaultTimestepUs = 0>
    struct TimeSeries {
        dsfw::TimePos timestep = DefaultTimestepUs;
        std::vector<T> values;

        dsfw::TimePos totalDuration() const {
            if (values.empty())
                return 0;
            return static_cast<dsfw::TimePos>(values.size()) * timestep;
        }

        int count() const {
            return static_cast<int>(values.size());
        }

        int sampleCount() const {
            return count();
        }

        int frameCount() const {
            return count();
        }

        T getValueAtTime(dsfw::TimePos timeUs) const {
            if (values.empty() || timestep <= 0)
                return T{};
            double index = static_cast<double>(timeUs) / static_cast<double>(timestep);
            int i = static_cast<int>(index);
            if (i < 0)
                return values.front();
            if (i >= static_cast<int>(values.size()) - 1)
                return values.back();
            double frac = index - i;
            if constexpr (std::is_integral_v<T>) {
                return static_cast<T>(std::lround(static_cast<double>(values[i]) * (1.0 - frac) +
                                                  static_cast<double>(values[i + 1]) * frac));
            } else {
                return static_cast<T>(values[i] * (1.0 - frac) + values[i + 1] * frac);
            }
        }

        std::vector<T> getRange(dsfw::TimePos startTimeUs, dsfw::TimePos endTimeUs) const {
            if (values.empty() || timestep <= 0)
                return {};
            int startIdx = static_cast<int>(startTimeUs / timestep);
            int endIdx = static_cast<int>(endTimeUs / timestep) + 1;
            startIdx = std::max(0, startIdx);
            endIdx = std::min(static_cast<int>(values.size()), endIdx);
            if (startIdx >= endIdx)
                return {};
            return std::vector<T>(values.begin() + startIdx, values.begin() + endIdx);
        }

        void setRange(dsfw::TimePos startTimeUs, const std::vector<T> &newValues) {
            if (values.empty() || timestep <= 0)
                return;
            int startIdx = static_cast<int>(startTimeUs / timestep);
            for (size_t i = 0; i < newValues.size(); ++i) {
                int idx = startIdx + static_cast<int>(i);
                if (idx >= 0 && idx < static_cast<int>(values.size())) {
                    values[idx] = newValues[i];
                }
            }
        }

        bool isEmpty() const {
            return values.empty();
        }

        double durationSec() const {
            return dsfw::usToSec(totalDuration());
        }
    };

    using F0Curve = TimeSeries<float>;
    using MouthCurve = TimeSeries<float, 20000>;

} // namespace dsfw::signal