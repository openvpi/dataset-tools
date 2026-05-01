#pragma once

/// @file IPitchService.h
/// @brief Pitch extraction service interface and result types.

#include <QString>
#include <dstools/Result.h>
#include <functional>
#include <vector>

namespace dstools {

/// @brief F0 extraction output.
struct PitchResult {
    std::vector<float> f0; ///< Fundamental frequency values.
    double hopMs = 0.0;    ///< Hop size in milliseconds.
    int sampleRate = 0;    ///< Source audio sample rate in Hz.
};

/// @brief Abstract interface for pitch (F0) extraction backends (e.g. RMVPE).
class IPitchService {
public:
    virtual ~IPitchService() = default;

    /// @brief Load the pitch extraction model.
    /// @param modelPath Path to the model file.
    /// @param gpuIndex GPU device index, or -1 for CPU.
    /// @return Ok on success, Err with description on failure.
    virtual Result<void> loadModel(const QString &modelPath, int gpuIndex = -1) = 0;

    /// @brief Check whether a model is currently loaded.
    /// @return True if a model is loaded and ready.
    virtual bool isModelLoaded() const = 0;

    /// @brief Unload the current model and free resources.
    virtual void unloadModel() = 0;

    /// @brief Extract the F0 curve from an audio file.
    /// @param audioPath Path to the input audio file.
    /// @return PitchResult on success, Err on failure.
    virtual Result<PitchResult> extractPitch(const QString &audioPath) = 0;

    /// @brief Extract F0 with per-frame output and progress reporting.
    /// @param audioPath Path to the input audio file.
    /// @param f0Frames Output vector of per-frame F0 values.
    /// @param progress Optional progress callback receiving percentage (0-100).
    /// @return Ok on success, Err on failure.
    virtual Result<void> extractPitchWithProgress(const QString &audioPath,
                                                   std::vector<std::vector<float>> &f0Frames,
                                                   const std::function<void(int)> &progress = nullptr) = 0;
};

} // namespace dstools
