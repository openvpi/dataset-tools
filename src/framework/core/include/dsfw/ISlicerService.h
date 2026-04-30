#pragma once

/// @file ISlicerService.h
/// @brief Audio slicing service interface.

#include <QString>

#include <dstools/Result.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace dstools {

/// @brief Result of an audio slicing operation.
struct SliceResult {
    std::vector<std::pair<int64_t, int64_t>> chunks; ///< Start/end sample pairs.
    int sampleRate = 0;                              ///< Sample rate of the source audio.
};

/// @brief Abstract interface for audio slicing backends.
class ISlicerService {
public:
    virtual ~ISlicerService() = default;

    /// @brief Slice an audio file into chunks based on silence detection.
    /// @param audioPath Path to the input audio file.
    /// @param threshold RMS threshold for silence detection (dB).
    /// @param minLength Minimum chunk length in milliseconds.
    /// @param minInterval Minimum silence interval in milliseconds.
    /// @param hopSize Hop size in samples for RMS calculation.
    /// @return SliceResult on success, or an error description.
    virtual Result<SliceResult> slice(const QString &audioPath, double threshold, int minLength,
                                     int minInterval, int hopSize) = 0;
};

} // namespace dstools
