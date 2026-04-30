#include <dstools/F0Curve.h>
#include <algorithm>

namespace dstools {

double F0Curve::totalDuration() const {
    if (values.empty()) return 0.0;
    return static_cast<double>(values.size()) * timestep;
}

double F0Curve::getValueAtTime(double time) const {
    if (values.empty() || timestep <= 0) return 0.0;
    double index = time / timestep;
    int i = static_cast<int>(index);
    if (i < 0) return values.front();
    if (i >= static_cast<int>(values.size()) - 1) return values.back();
    double frac = index - i;
    return values[i] * (1.0 - frac) + values[i + 1] * frac;
}

std::vector<double> F0Curve::getRange(double startTime, double endTime) const {
    if (values.empty() || timestep <= 0) return {};
    int startIdx = static_cast<int>(startTime / timestep);
    int endIdx = static_cast<int>(endTime / timestep) + 1;
    startIdx = std::max(0, startIdx);
    endIdx = std::min(static_cast<int>(values.size()), endIdx);
    if (startIdx >= endIdx) return {};
    return std::vector<double>(values.begin() + startIdx, values.begin() + endIdx);
}

void F0Curve::setRange(double startTime, const std::vector<double> &newValues) {
    if (values.empty() || timestep <= 0) return;
    int startIdx = static_cast<int>(startTime / timestep);
    for (size_t i = 0; i < newValues.size(); ++i) {
        int idx = startIdx + static_cast<int>(i);
        if (idx >= 0 && idx < static_cast<int>(values.size())) {
            values[idx] = newValues[i];
        }
    }
}

} // namespace dstools
