#pragma once

/// @file ITaskProcessor.h
/// @brief Unified interface for all model-backed processing tasks.

#include <dsfw/TaskTypes.h>
#include <dstools/Result.h>

#include <memory>

namespace dstools {

class IModelManager;

/// @brief Unified processor interface replacing per-domain service interfaces.
///
/// Each processor declares its I/O contract via taskSpec(), exposes configurable
/// parameters via capabilities(), and supports both interactive (process) and
/// batch (processBatch) execution modes.
///
/// @see TaskProcessorRegistry for discovery and instantiation.
/// @see Essentia Algorithm + SOFA BaseG2P for design inspiration.
class ITaskProcessor {
public:
    virtual ~ITaskProcessor() = default;

    // ── Metadata ──

    /// @brief Unique processor identifier (e.g. "hubert-fa", "rmvpe", "game").
    virtual QString processorId() const = 0;

    /// @brief Human-readable display name.
    virtual QString displayName() const = 0;

    /// @brief I/O specification (corresponds to .dsproj tasks[]).
    virtual TaskSpec taskSpec() const = 0;

    /// @brief Parameter capabilities declaration.
    /// Returns JSON describing supported parameters, their types, ranges, and defaults.
    /// UI uses this to dynamically generate configuration controls.
    virtual ProcessorConfig capabilities() const { return {}; }

    // ── Model lifecycle ──

    /// @brief Initialize the processor: register and load models via ModelManager.
    /// @param mm Model manager for model lifecycle.
    /// @param modelConfig Engine-specific model configuration.
    /// @return Success or error.
    virtual Result<void> initialize(IModelManager &mm,
                                    const ProcessorConfig &modelConfig) = 0;

    /// @brief Release processor internal state.
    /// Models are managed by ModelManager; this releases processor-specific resources.
    virtual void release() = 0;

    // ── Processing ──

    /// @brief Process a single item (interactive mode).
    virtual Result<TaskOutput> process(const TaskInput &input) = 0;

    /// @brief Process a batch of items (pipeline mode).
    /// Default: iterates process() over items. Override for native batch APIs.
    virtual Result<BatchOutput> processBatch(const BatchInput &input,
                                             ProgressCallback progress = nullptr);
};

} // namespace dstools
