#pragma once

/// @file CurveTools.h
/// @brief Curve interpolation, resampling, and processing utilities.
///
/// Designed for F0 curves but applicable to any equally-spaced time-series data.
/// All time values in microseconds (TimePos). All F0 values in millihertz (int32_t).

#include <dstools/TimePos.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace dstools {

// ═══════════════════════════════════════════════════════════════════
// 1. Resampling
// ═══════════════════════════════════════════════════════════════════

/// Resample an equally-spaced curve from srcTimestep to dstTimestep via linear interpolation.
/// Equivalent to MakeDiffSinger get_pitch.py resample_align_curve().
/// Tail beyond source range is filled with the last value.
/// @param alignLength  Target output length. 0 = auto-calculate.
std::vector<int32_t> resampleCurve(const std::vector<int32_t> &values,
                                    TimePos srcTimestep,
                                    TimePos dstTimestep,
                                    int alignLength = 0);

/// Same as above, double version (Hz domain or other float curves).
std::vector<double> resampleCurveF(const std::vector<double> &values,
                                    TimePos srcTimestep,
                                    TimePos dstTimestep,
                                    int alignLength = 0);

/// Compute timestep in microseconds from hop_size and sample_rate.
inline TimePos hopSizeToTimestep(int hopSize, int sampleRate) {
    return static_cast<TimePos>(
        std::llround(static_cast<double>(hopSize) / sampleRate * 1e6));
}

/// Expected F0 frame count from audio sample count and hop_size.
inline int expectedFrameCount(int64_t audioSamples, int hopSize) {
    return static_cast<int>((audioSamples + hopSize - 1) / hopSize);
}

// ═══════════════════════════════════════════════════════════════════
// 2. Unvoiced interpolation
// ═══════════════════════════════════════════════════════════════════

/// Interpolate unvoiced frames (value == 0) in log2(Hz) domain.
/// Edge unvoiced regions are filled with the nearest voiced value.
/// All-unvoiced input is left unmodified.
/// @param[in,out] values  F0 in mHz. 0 = unvoiced. Modified in-place.
/// @param[out]    uv      Optional unvoiced mask output (true = originally unvoiced).
void interpUnvoiced(std::vector<int32_t> &values,
                    std::vector<bool> *uv = nullptr);

/// Same, double/Hz version.
void interpUnvoicedF(std::vector<double> &values,
                     std::vector<bool> *uv = nullptr);

// ═══════════════════════════════════════════════════════════════════
// 3. Smoothing filters
// ═══════════════════════════════════════════════════════════════════

/// Moving average filter. Skips zero-valued frames by default (F0 unvoiced).
std::vector<double> movingAverage(const std::vector<double> &values,
                                   int window,
                                   bool skipZero = true);

/// Median filter.
std::vector<double> medianFilter(const std::vector<double> &values,
                                  int window);

// ═══════════════════════════════════════════════════════════════════
// 4. Batch conversions
// ═══════════════════════════════════════════════════════════════════

std::vector<int32_t> hzToMhzBatch(const std::vector<double> &hz);
std::vector<double> mhzToHzBatch(const std::vector<int32_t> &mhz);
std::vector<double> hzToMidiBatch(const std::vector<double> &hz);
std::vector<double> midiToHzBatch(const std::vector<double> &midi);
std::vector<double> mhzToMidiBatch(const std::vector<int32_t> &mhz);

// ═══════════════════════════════════════════════════════════════════
// 5. Alignment
// ═══════════════════════════════════════════════════════════════════

/// Truncate or pad a curve to targetLength. Padding uses last value (not fillValue)
/// when the source is non-empty, matching MDS resample_align_curve behaviour.
template <typename T>
std::vector<T> alignToLength(const std::vector<T> &values,
                              int targetLength,
                              T fillValue = T{}) {
    std::vector<T> result(targetLength, fillValue);
    const int copyLen = std::min(static_cast<int>(values.size()), targetLength);
    std::copy_n(values.begin(), copyLen, result.begin());
    if (targetLength > static_cast<int>(values.size()) && !values.empty()) {
        std::fill(result.begin() + copyLen, result.end(), values.back());
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════
// 6. Smoothstep crossfade
// ═══════════════════════════════════════════════════════════════════

/// Apply smoothstep crossfade at head/tail of a curve against an original.
/// Used to avoid hard-cut artifacts when splicing curves.
void smoothstepCrossfade(std::vector<double> &values,
                          const std::vector<double> &original,
                          int crossfadeLen);

} // namespace dstools
