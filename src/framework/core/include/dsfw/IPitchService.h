#pragma once

/// @file IPitchService.h
/// @brief Pitch (F0) extraction service interface.

#include <QString>

#include <dstools/Result.h>

#include <vector>

namespace dstools {

/// @brief Result of a pitch extraction operation.
struct PitchResult {
    std::vector<float> f0; ///< F0 values in Hz (0 for unvoiced frames).
    double hopMs = 0.0;    ///< Hop size in milliseconds between frames.
    int sampleRate = 0;    ///< Sample rate of the source audio.
};

/// @brief Abstract interface for pitch extraction backends.
class IPitchService {
public:
    virtual ~IPitchService() = default;

    /// @brief Extract the F0 contour from an audio file.
    /// @param audioPath Path to the input audio file.
    /// @return PitchResult on success, or an error description.
    virtual Result<PitchResult> extractPitch(const QString &audioPath) = 0;
};

} // namespace dstools
