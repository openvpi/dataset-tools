#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <dstools/TimePos.h>
#include <vector>

namespace dsfw::signal {

    // ═══════════════════════════════════════════════════════════════════
    // 1. Resampling
    // ═══════════════════════════════════════════════════════════════════

    std::vector<float> resampleCurve(const std::vector<float> &values, dstools::TimePos srcTimestep,
                                     dstools::TimePos dstTimestep, int alignLength = 0);

    std::vector<double> resampleCurveF(const std::vector<double> &values, dstools::TimePos srcTimestep,
                                       dstools::TimePos dstTimestep, int alignLength = 0);

    inline dstools::TimePos hopSizeToTimestep(int hopSize, int sampleRate) {
        return std::llround(static_cast<double>(hopSize) / sampleRate * 1e6);
    }

    inline int expectedFrameCount(int64_t audioSamples, int hopSize) {
        return static_cast<int>((audioSamples + hopSize - 1) / hopSize);
    }

    // ═══════════════════════════════════════════════════════════════════
    // 2. Unvoiced interpolation
    // ═══════════════════════════════════════════════════════════════════

    void interpUnvoiced(std::vector<int32_t> &values, std::vector<bool> *uv = nullptr);

    void interpUnvoicedF(std::vector<double> &values, std::vector<bool> *uv = nullptr);

    // ═══════════════════════════════════════════════════════════════════
    // 3. Smoothing filters
    // ═══════════════════════════════════════════════════════════════════

    std::vector<double> movingAverage(const std::vector<double> &values, int window, bool skipZero = true);

    std::vector<double> medianFilter(const std::vector<double> &values, int window);

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

    template <typename T>
    std::vector<T> alignToLength(const std::vector<T> &values, int targetLength, T fillValue = T{}) {
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

    void smoothstepCrossfade(std::vector<double> &values, const std::vector<double> &original, int crossfadeLen);

    // ═══════════════════════════════════════════════════════════════════
    // 7. Boundary dragging
    // ═══════════════════════════════════════════════════════════════════

    /**
     * @brief Drag a boundary point in a curve with Gaussian influence
     *
     * This function applies a Gaussian-weighted adjustment to values around a boundary point.
     * The adjustment follows a Gaussian distribution centered at the boundary index,
     * with influence radius controlling the spread of the effect.
     *
     * @param values The curve values to modify
     * @param boundaryIdx The index of the boundary point to drag (0-based)
     * @param delta The amount to drag the boundary (positive or negative)
     * @param influenceRadius The radius of influence around the boundary (default: 10)
     *
     * @note The Gaussian weight is computed as exp(-distance²/(2σ²)) where σ = influenceRadius/3
     * @note Values outside the influence radius are not modified
     * @note If boundaryIdx is out of bounds, the function does nothing
     */
    void dragBoundary(std::vector<double> &values, int boundaryIdx, double delta, int influenceRadius = 10);

    /**
     * @brief Drag a boundary point in a curve with Gaussian influence (int32_t version)
     *
     * This function applies a Gaussian-weighted adjustment to integer values around a boundary point.
     * The adjustment follows a Gaussian distribution centered at the boundary index,
     * with influence radius controlling the spread of the effect.
     *
     * @param values The curve values to modify
     * @param boundaryIdx The index of the boundary point to drag (0-based)
     * @param delta The amount to drag the boundary (positive or negative)
     * @param influenceRadius The radius of influence around the boundary (default: 10)
     *
     * @note The Gaussian weight is computed as exp(-distance²/(2σ²)) where σ = influenceRadius/3
     * @note Values are rounded to the nearest integer after applying the weight
     * @note Values outside the influence radius are not modified
     * @note If boundaryIdx is out of bounds, the function does nothing
     */
    void dragBoundary(std::vector<int32_t> &values, int boundaryIdx, int32_t delta, int influenceRadius = 10);

} // namespace dsfw::signal
