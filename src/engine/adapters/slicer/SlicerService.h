#pragma once

/// @file SlicerService.h
/// @brief Audio slicer service implementation using RMS-based silence detection.

#include <QString>
#include <cstdint>
#include <dsfw/Result.h>
#include <utility>
#include <vector>

namespace dsfw::signal {
    struct SlicerParams;
}

class SndfileHandle;

namespace dstools {

    using namespace dsfw;

    struct SliceResult {
        std::vector<std::pair<int64_t, int64_t>> chunks;
        int sampleRate = 0;
    };

    class SlicerService {
    public:
        SlicerService() = default;
        ~SlicerService() = default;

        Result<SliceResult> slice(const QString &audioPath, double threshold, int minLength,
                                  int minInterval, int hopSize, int maxSilKept = 5000);

        static std::vector<double> computeSlicePoints(const std::vector<float> &samples, int sampleRate,
                                                      const dsfw::signal::SlicerParams &params);

        static std::vector<float> loadAndMixAudio(const QString &filePath, int *outSampleRate);
    };

} // namespace dstools