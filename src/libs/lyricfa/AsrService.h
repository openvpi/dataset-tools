#pragma once

/// @file AsrService.h
/// @brief ASR service implementation bridging FunASR to the IAsrService interface.

#include <dsfw/IAsrService.h>

#include <memory>
#include <mutex>

namespace FunAsr {
    class Model;
}

namespace LyricFA {
    class Asr;
}

/// @brief IAsrService implementation using LyricFA::Asr for speech recognition.
class AsrService : public dstools::IAsrService {
public:
    AsrService() = default;
    ~AsrService() override;

    /// @brief Load the ASR model from the given path.
    /// @param modelPath Path to the model directory.
    /// @param gpuIndex GPU device index (-1 for CPU).
    /// @return Result indicating success or failure.
    dstools::Result<void> loadModel(const QString &modelPath, int gpuIndex = -1) override;

    /// @brief Check if the ASR model is loaded.
    /// @return True if loaded.
    bool isModelLoaded() const override;

    /// @brief Unload the ASR model and release resources.
    void unloadModel() override;

    /// @brief Recognize speech from an audio file.
    /// @param audioPath Path to the audio file.
    /// @return Recognition result or error.
    dstools::Result<dstools::AsrResult> recognize(const QString &audioPath) override;

private:
    std::unique_ptr<LyricFA::Asr> m_asr; ///< ASR engine instance.
    mutable std::mutex m_mutex;           ///< Mutex for thread-safe access.
};
