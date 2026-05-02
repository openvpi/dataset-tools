/// @file CurveTools.cpp
/// @brief Implementation of curve interpolation, resampling, and processing utilities.

#include <dstools/CurveTools.h>

#include <algorithm>
#include <cmath>
#include <numeric>

namespace dstools {

// ═══════════════════════════════════════════════════════════════════
// 1. Resampling
// ═══════════════════════════════════════════════════════════════════

std::vector<int32_t> resampleCurve(const std::vector<int32_t> &values,
                                    TimePos srcTimestep,
                                    TimePos dstTimestep,
                                    int alignLength) {
    if (values.empty() || srcTimestep <= 0 || dstTimestep <= 0)
        return {};

    const double tMax = static_cast<double>(values.size() - 1) * srcTimestep;
    const int outLen = alignLength > 0
        ? alignLength
        : static_cast<int>(std::ceil(tMax / dstTimestep)) + 1;

    std::vector<int32_t> result(outLen);
    for (int i = 0; i < outLen; ++i) {
        const double t = static_cast<double>(i) * dstTimestep;
        const double srcIdx = t / static_cast<double>(srcTimestep);

        int lo = static_cast<int>(srcIdx);
        if (lo >= static_cast<int>(values.size()) - 1) {
            result[i] = values.back();
            continue;
        }
        if (lo < 0) lo = 0;

        const double frac = srcIdx - lo;
        result[i] = static_cast<int32_t>(std::lround(
            values[lo] * (1.0 - frac) + values[lo + 1] * frac));
    }
    return result;
}

std::vector<double> resampleCurveF(const std::vector<double> &values,
                                    TimePos srcTimestep,
                                    TimePos dstTimestep,
                                    int alignLength) {
    if (values.empty() || srcTimestep <= 0 || dstTimestep <= 0)
        return {};

    const double tMax = static_cast<double>(values.size() - 1) * srcTimestep;
    const int outLen = alignLength > 0
        ? alignLength
        : static_cast<int>(std::ceil(tMax / dstTimestep)) + 1;

    std::vector<double> result(outLen);
    for (int i = 0; i < outLen; ++i) {
        const double t = static_cast<double>(i) * dstTimestep;
        const double srcIdx = t / static_cast<double>(srcTimestep);

        int lo = static_cast<int>(srcIdx);
        if (lo >= static_cast<int>(values.size()) - 1) {
            result[i] = values.back();
            continue;
        }
        if (lo < 0) lo = 0;

        const double frac = srcIdx - lo;
        result[i] = values[lo] * (1.0 - frac) + values[lo + 1] * frac;
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════
// 2. Unvoiced interpolation
// ═══════════════════════════════════════════════════════════════════

void interpUnvoiced(std::vector<int32_t> &values, std::vector<bool> *uvOut) {
    const int n = static_cast<int>(values.size());
    if (n == 0) return;

    std::vector<bool> uv(n);
    for (int i = 0; i < n; ++i)
        uv[i] = (values[i] == 0);

    if (uvOut) *uvOut = uv;

    // Check if any voiced frame exists
    int first = -1;
    for (int i = 0; i < n; ++i) {
        if (!uv[i]) { first = i; break; }
    }
    if (first < 0) return; // all unvoiced

    int last = n - 1;
    while (last >= 0 && uv[last]) --last;

    // Fill edges with nearest voiced value
    for (int i = 0; i < first; ++i)
        values[i] = values[first];
    for (int i = last + 1; i < n; ++i)
        values[i] = values[last];

    // Interior: log2 domain linear interpolation
    for (int i = first; i <= last; ++i) {
        if (!uv[i]) continue;

        const int prev = i - 1; // guaranteed voiced
        int next = i + 1;
        while (next <= last && uv[next]) ++next;
        if (next > last) break;

        const double logPrev = std::log2(mhzToHz(values[prev]));
        const double logNext = std::log2(mhzToHz(values[next]));

        for (int j = i; j < next; ++j) {
            if (!uv[j]) continue;
            const double t = static_cast<double>(j - prev) / (next - prev);
            const double logInterp = logPrev + t * (logNext - logPrev);
            values[j] = hzToMhz(std::pow(2.0, logInterp));
        }
        i = next - 1;
    }
}

void interpUnvoicedF(std::vector<double> &values, std::vector<bool> *uvOut) {
    const int n = static_cast<int>(values.size());
    if (n == 0) return;

    std::vector<bool> uv(n);
    for (int i = 0; i < n; ++i)
        uv[i] = (values[i] == 0.0);

    if (uvOut) *uvOut = uv;

    int first = -1;
    for (int i = 0; i < n; ++i) {
        if (!uv[i]) { first = i; break; }
    }
    if (first < 0) return;

    int last = n - 1;
    while (last >= 0 && uv[last]) --last;

    for (int i = 0; i < first; ++i)
        values[i] = values[first];
    for (int i = last + 1; i < n; ++i)
        values[i] = values[last];

    for (int i = first; i <= last; ++i) {
        if (!uv[i]) continue;

        const int prev = i - 1;
        int next = i + 1;
        while (next <= last && uv[next]) ++next;
        if (next > last) break;

        const double logPrev = std::log2(values[prev]);
        const double logNext = std::log2(values[next]);

        for (int j = i; j < next; ++j) {
            if (!uv[j]) continue;
            const double t = static_cast<double>(j - prev) / (next - prev);
            values[j] = std::pow(2.0, logPrev + t * (logNext - logPrev));
        }
        i = next - 1;
    }
}

// ═══════════════════════════════════════════════════════════════════
// 3. Smoothing filters
// ═══════════════════════════════════════════════════════════════════

std::vector<double> movingAverage(const std::vector<double> &values,
                                   int window,
                                   bool skipZero) {
    const int n = static_cast<int>(values.size());
    if (n == 0 || window <= 0) return values;
    if (window % 2 == 0) ++window; // ensure odd

    const int half = window / 2;
    std::vector<double> result(n);

    for (int i = 0; i < n; ++i) {
        if (skipZero && values[i] == 0.0) {
            result[i] = 0.0;
            continue;
        }
        double sum = 0.0;
        int count = 0;
        for (int j = i - half; j <= i + half; ++j) {
            if (j < 0 || j >= n) continue;
            if (skipZero && values[j] == 0.0) continue;
            sum += values[j];
            ++count;
        }
        result[i] = count > 0 ? sum / count : values[i];
    }
    return result;
}

std::vector<double> medianFilter(const std::vector<double> &values, int window) {
    const int n = static_cast<int>(values.size());
    if (n == 0 || window <= 0) return values;
    if (window % 2 == 0) ++window;

    const int half = window / 2;
    std::vector<double> result(n);
    std::vector<double> buf;

    for (int i = 0; i < n; ++i) {
        buf.clear();
        for (int j = i - half; j <= i + half; ++j) {
            if (j < 0 || j >= n) continue;
            buf.push_back(values[j]);
        }
        std::sort(buf.begin(), buf.end());
        result[i] = buf[buf.size() / 2];
    }
    return result;
}

// ═══════════════════════════════════════════════════════════════════
// 4. Batch conversions
// ═══════════════════════════════════════════════════════════════════

std::vector<int32_t> hzToMhzBatch(const std::vector<double> &hz) {
    std::vector<int32_t> out(hz.size());
    for (size_t i = 0; i < hz.size(); ++i)
        out[i] = (hz[i] == 0.0) ? 0 : hzToMhz(hz[i]);
    return out;
}

std::vector<double> mhzToHzBatch(const std::vector<int32_t> &mhz) {
    std::vector<double> out(mhz.size());
    for (size_t i = 0; i < mhz.size(); ++i)
        out[i] = mhzToHz(mhz[i]);
    return out;
}

std::vector<double> hzToMidiBatch(const std::vector<double> &hz) {
    std::vector<double> out(hz.size());
    for (size_t i = 0; i < hz.size(); ++i)
        out[i] = (hz[i] <= 0.0) ? 0.0 : 12.0 * std::log2(hz[i] / 440.0) + 69.0;
    return out;
}

std::vector<double> midiToHzBatch(const std::vector<double> &midi) {
    std::vector<double> out(midi.size());
    for (size_t i = 0; i < midi.size(); ++i)
        out[i] = 440.0 * std::pow(2.0, (midi[i] - 69.0) / 12.0);
    return out;
}

std::vector<double> mhzToMidiBatch(const std::vector<int32_t> &mhz) {
    std::vector<double> out(mhz.size());
    for (size_t i = 0; i < mhz.size(); ++i) {
        const double hz = mhzToHz(mhz[i]);
        out[i] = (hz <= 0.0) ? 0.0 : 12.0 * std::log2(hz / 440.0) + 69.0;
    }
    return out;
}

// ═══════════════════════════════════════════════════════════════════
// 6. Smoothstep crossfade
// ═══════════════════════════════════════════════════════════════════

static double smoothstep(double t) {
    t = std::clamp(t, 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

void smoothstepCrossfade(std::vector<double> &values,
                          const std::vector<double> &original,
                          int crossfadeLen) {
    const int n = static_cast<int>(values.size());
    if (n == 0 || crossfadeLen <= 0) return;

    const int origN = static_cast<int>(original.size());
    const int fadeIn = std::min(crossfadeLen, n);
    const int fadeOut = std::min(crossfadeLen, n);

    // Fade in: blend from original to values
    for (int i = 0; i < fadeIn && i < origN; ++i) {
        const double t = smoothstep(static_cast<double>(i) / crossfadeLen);
        values[i] = original[i] * (1.0 - t) + values[i] * t;
    }

    // Fade out: blend from values back to original
    for (int i = 0; i < fadeOut; ++i) {
        const int idx = n - 1 - i;
        if (idx < 0 || idx >= origN) continue;
        const double t = smoothstep(static_cast<double>(i) / crossfadeLen);
        values[idx] = original[idx] * (1.0 - t) + values[idx] * t;
    }
}

} // namespace dstools
