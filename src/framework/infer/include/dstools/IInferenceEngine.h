#pragma once
/// @file IInferenceEngine.h
/// @brief Abstract interface for inference engine implementations.

#include <cstdint>
#include <filesystem>
#include <string>

#include <dstools/ExecutionProvider.h>
#include <dstools/Result.h>

namespace dstools::infer {

    /// @brief Abstract interface for model inference engines.
    class IInferenceEngine {
    public:
        virtual ~IInferenceEngine() = default;

        /// @brief Check whether a model is loaded.
        /// @return True if a model is currently loaded.
        virtual bool isOpen() const = 0;

        /// @brief Request early termination of a running inference.
        virtual void terminate() {}

        /// @brief Get the engine name for identification.
        /// @return Engine name string.
        virtual const char *engineName() const = 0;

        /// @brief Load a model from disk.
        /// @param modelPath Path to the model file.
        /// @param provider Execution provider (CPU, DML, CUDA).
        /// @param deviceId Device index for GPU providers.
        /// @return Success or error.
        virtual Result<void> load(const std::filesystem::path &modelPath,
                                  ExecutionProvider provider, int deviceId) = 0;

        /// @brief Unload the current model and free resources.
        virtual void unload() {}

        /// @brief Estimate memory usage of the loaded model.
        /// @return Estimated memory in bytes, or 0 if unknown.
        virtual int64_t estimatedMemoryBytes() const { return 0; }
    };

} // namespace dstools::infer
