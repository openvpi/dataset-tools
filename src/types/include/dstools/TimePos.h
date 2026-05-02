#pragma once

/// @file TimePos.h
/// @brief Integer microsecond time representation for zero-drift precision.

#include <cmath>
#include <cstdint>

namespace dstools {

/// All time positions and durations in microseconds (1 s = 1'000'000 μs).
/// Eliminates floating-point accumulation errors across boundary binding and I/O.
using TimePos = int64_t;

/// 1 second in TimePos units.
inline constexpr TimePos kMicrosecondsPerSecond = 1'000'000;

/// 1 millisecond in TimePos units.
inline constexpr TimePos kMicrosecondsPerMs = 1'000;

/// Convert floating-point seconds to TimePos (rounds to nearest μs).
inline TimePos secToUs(double sec) {
    return static_cast<TimePos>(std::llround(sec * 1e6));
}

/// Convert TimePos to floating-point seconds.
inline double usToSec(TimePos us) {
    return static_cast<double>(us) / 1e6;
}

/// Convert TimePos to floating-point milliseconds.
inline double usToMs(TimePos us) {
    return static_cast<double>(us) / 1e3;
}

/// Convert floating-point milliseconds to TimePos.
inline TimePos msToUs(double ms) {
    return static_cast<TimePos>(std::llround(ms * 1e3));
}

// ── F0 value helpers ──

/// Convert Hz (double) to millihertz (int32_t). 261.6 Hz → 261600 mHz.
inline int32_t hzToMhz(double hz) {
    return static_cast<int32_t>(std::lround(hz * 1000.0));
}

/// Convert millihertz (int32_t) to Hz (double). 261600 → 261.6.
inline double mhzToHz(int32_t mhz) {
    return static_cast<double>(mhz) / 1000.0;
}

} // namespace dstools
