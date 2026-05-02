#pragma once

#include <dstools/TimePos.h>

#include <cstdint>
#include <vector>

namespace dstools {

struct F0Curve {
    TimePos timestep = 0;                    ///< Microseconds between samples
    std::vector<int32_t> values;             ///< F0 in millihertz, 0 = unvoiced

    TimePos totalDuration() const;

    int sampleCount() const { return static_cast<int>(values.size()); }

    int32_t getValueAtTime(TimePos timeUs) const;

    std::vector<int32_t> getRange(TimePos startTimeUs, TimePos endTimeUs) const;

    void setRange(TimePos startTimeUs, const std::vector<int32_t> &newValues);

    bool isEmpty() const { return values.empty(); }
};

} // namespace dstools
