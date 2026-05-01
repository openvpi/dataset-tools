#pragma once

/// @file RmvpePitchService.h
/// @brief RMVPE-based pitch extraction service implementation.

#include <dsfw/IPitchService.h>

#include <memory>
#include <mutex>

namespace Rmvpe {
class Rmvpe;
}

namespace dstools {

/// @brief IPitchService implementation using the RMVPE model for F0 extraction.
class RmvpePitchService : public IPitchService {
public:
    RmvpePitchService();
    ~RmvpePitchService() override;

    /// @brief Load an RMVPE pitch model.
    /// @param modelPath Path to the model file.
    /// @param gpuIndex GPU device index, -1 for CPU.
    /// @return Success or error.
    Result<void> loadModel(const QString &modelPath, int gpuIndex = -1) override;

    /// @brief Check if a model is loaded and ready.
    /// @return True if a model is loaded.
    bool isModelLoaded() const override;

    /// @brief Release model resources.
    void unloadModel() override;

    /// @brief Extract pitch (F0) from an audio file.
    /// @param audioPath Path to the audio file.
    /// @return Pitch result or error.
    Result<PitchResult> extractPitch(const QString &audioPath) override;

    /// @brief Extract pitch with progress reporting.
    /// @param audioPath Path to the audio file.
    /// @param f0Frames Output F0 frames.
    /// @param progress Progress callback receiving percentage.
    /// @return Success or error.
    Result<void> extractPitchWithProgress(const QString &audioPath,
                                           std::vector<std::vector<float>> &f0Frames,
                                           const std::function<void(int)> &progress = nullptr) override;

private:
    std::unique_ptr<Rmvpe::Rmvpe> m_rmvpe; ///< RMVPE engine instance
    mutable std::mutex m_mutex;             ///< Serializes access to the engine
};

} // namespace dstools
