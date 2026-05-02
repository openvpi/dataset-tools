#include <dstools/F0Curve.h>
#include <dstools/CurveTools.h>

#include <algorithm>
#include <cmath>

namespace dstools {

TimePos F0Curve::totalDuration() const {
    if (values.empty()) return 0;
    return static_cast<TimePos>(values.size()) * timestep;
}

int32_t F0Curve::getValueAtTime(TimePos timeUs) const {
    if (values.empty() || timestep <= 0) return 0;
    double index = static_cast<double>(timeUs) / static_cast<double>(timestep);
    int i = static_cast<int>(index);
    if (i < 0) return values.front();
    if (i >= static_cast<int>(values.size()) - 1) return values.back();
    double frac = index - i;
    return static_cast<int32_t>(std::lround(
        values[i] * (1.0 - frac) + values[i + 1] * frac));
}

std::vector<int32_t> F0Curve::getRange(TimePos startTimeUs, TimePos endTimeUs) const {
    if (values.empty() || timestep <= 0) return {};
    int startIdx = static_cast<int>(startTimeUs / timestep);
    int endIdx = static_cast<int>(endTimeUs / timestep) + 1;
    startIdx = std::max(0, startIdx);
    endIdx = std::min(static_cast<int>(values.size()), endIdx);
    if (startIdx >= endIdx) return {};
    return std::vector<int32_t>(values.begin() + startIdx, values.begin() + endIdx);
}

void F0Curve::setRange(TimePos startTimeUs, const std::vector<int32_t> &newValues) {
    if (values.empty() || timestep <= 0) return;
    int startIdx = static_cast<int>(startTimeUs / timestep);
    for (size_t i = 0; i < newValues.size(); ++i) {
        int idx = startIdx + static_cast<int>(i);
        if (idx >= 0 && idx < static_cast<int>(values.size())) {
            values[idx] = newValues[i];
        }
    }
}

} // namespace dstools
