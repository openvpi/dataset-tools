#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include <dstools/ExecutionProvider.h>

namespace dstools::infer {

    /// Abstract interface representing the common lifecycle shared by all
    /// inference engines (GAME, RMVPE, HuBERT-FA).
    class IInferenceEngine {
    public:
        virtual ~IInferenceEngine() = default;

        /// Whether the model is loaded and ready for inference.
        virtual bool isOpen() const = 0;

        /// Request graceful termination of ongoing inference.
        /// Default implementation does nothing (not all engines support this).
        virtual void terminate() {}

        /// Human-readable engine name (e.g. "GAME", "RMVPE", "HuBERT-FA").
        virtual const char *engineName() const = 0;

        /// Load model from \a modelPath using the given execution provider and
        /// device. On failure, writes a diagnostic into \a errorMsg and returns
        /// false.  Default implementation reports "not implemented".
        virtual bool load(const std::filesystem::path &modelPath,
                          ExecutionProvider provider, int deviceId,
                          std::string &errorMsg) {
            (void) modelPath;
            (void) provider;
            (void) deviceId;
            errorMsg = "load() not implemented";
            return false;
        }

        /// Release all model resources.  Default is a no-op so that existing
        /// engines are not forced to implement it immediately.
        virtual void unload() {}

        /// Estimated GPU/CPU memory footprint in bytes (0 = unknown).
        virtual int64_t estimatedMemoryBytes() const { return 0; }
    };

} // namespace dstools::infer
