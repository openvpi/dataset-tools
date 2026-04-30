#pragma once

/// @file IModelProvider.h
/// @brief Abstract interface for loading and managing ML inference models.

#include <QString>

#include <cstdint>
#include <functional>
#include <string>

#include <dstools/Result.h>

namespace dstools {

/// @brief Type of ML model.
enum class ModelType {
    Asr,    ///< Automatic speech recognition (FunASR).
    HuBERT, ///< HuBERT phoneme alignment model.
    GAME,   ///< GAME audio-to-MIDI transcription model.
    RMVPE,  ///< RMVPE pitch estimation model.
    SOME,   ///< SOME pitch estimation model.
    Custom  ///< Application-defined custom model type.
};

/// @brief Lifecycle status of a model.
enum class ModelStatus {
    Unloaded, ///< Model is not loaded.
    Loading,  ///< Model is currently being loaded.
    Ready,    ///< Model is loaded and ready for inference.
    Error     ///< Model failed to load.
};

/// @brief Callback for model loading progress updates.
/// @param current Bytes or steps completed.
/// @param total Total bytes or steps.
/// @param msg Human-readable status message.
using ModelProgressCallback =
    std::function<void(int64_t current, int64_t total, const QString &msg)>;

/// @brief Abstract interface for a single ML model's lifecycle.
class IModelProvider {
public:
    virtual ~IModelProvider() = default;
    /// @brief Return the model type.
    /// @return ModelType enum value.
    virtual ModelType type() const = 0;
    /// @brief Return a human-readable name for this model.
    /// @return Display name string.
    virtual QString displayName() const = 0;
    /// @brief Load the model from the given path.
    /// @param modelPath Path to the model directory or file.
    /// @param gpuIndex GPU device index (-1 for CPU).
    /// @return Success or error.
    virtual Result<void> load(const QString &modelPath, int gpuIndex) = 0;
    /// @brief Unload the model and free resources.
    virtual void unload() = 0;
    /// @brief Return the current model status.
    /// @return ModelStatus enum value.
    virtual ModelStatus status() const = 0;
    /// @brief Estimate the model's memory consumption in bytes.
    /// @return Estimated bytes, or 0 if unknown.
    virtual int64_t estimatedMemoryBytes() const = 0;
    /// @brief Convenience check for Ready status.
    /// @return True if status() == ModelStatus::Ready.
    bool isReady() const { return status() == ModelStatus::Ready; }
};

}
