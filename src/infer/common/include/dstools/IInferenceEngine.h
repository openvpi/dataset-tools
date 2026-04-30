#pragma once

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
    };

} // namespace dstools::infer
