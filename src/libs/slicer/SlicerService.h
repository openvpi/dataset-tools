#pragma once

/// @file SlicerService.h
/// @brief Audio slicer service implementation using RMS-based silence detection.

#include <dsfw/ISlicerService.h>

class SndfileHandle;

/// @brief ISlicerService implementation that splits audio files at silence boundaries.
class SlicerService : public dstools::ISlicerService {
public:
    SlicerService() = default;
    ~SlicerService() override = default;

    /// @brief Slice an audio file at silence boundaries.
    /// @param audioPath Path to the audio file.
    /// @param threshold RMS silence threshold in dB.
    /// @param minLength Minimum slice length in milliseconds.
    /// @param minInterval Minimum silence interval in milliseconds.
    /// @param hopSize Analysis hop size in samples.
    /// @param maxSilKept Maximum silence to keep at slice edges in milliseconds.
    /// @return Slice result or error.
    dstools::Result<dstools::SliceResult> slice(const QString &audioPath, double threshold, int minLength,
                                                int minInterval, int hopSize, int maxSilKept) override;
};
