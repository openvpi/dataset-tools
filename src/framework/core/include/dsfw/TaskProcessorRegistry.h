#pragma once

/// @file TaskProcessorRegistry.h
/// @brief Self-registering factory for ITaskProcessor implementations.

#include <dsfw/ITaskProcessor.h>

#include <QStringList>

#include <functional>
#include <map>
#include <memory>
#include <mutex>

namespace dstools {

/// @brief Factory function type for creating processor instances.
using ProcessorFactory = std::function<std::unique_ptr<ITaskProcessor>()>;

/// @brief Singleton registry mapping (taskName, processorId) → factory.
///
/// Processors register themselves via the Registrar helper at static initialization.
/// @see Essentia's EssentiaFactory::Registrar pattern.
class TaskProcessorRegistry {
public:
    /// @brief Get the singleton instance.
    static TaskProcessorRegistry &instance();

    /// @brief Register a processor factory for a task.
    /// @param taskName Task type (e.g. "phoneme_alignment").
    /// @param processorId Processor identifier (e.g. "hubert-fa").
    /// @param factory Factory function.
    void registerProcessor(const QString &taskName,
                           const QString &processorId,
                           ProcessorFactory factory);

    /// @brief Create a processor instance.
    /// @param taskName Task type.
    /// @param processorId Processor identifier.
    /// @return New processor instance, or nullptr if not found.
    std::unique_ptr<ITaskProcessor> create(const QString &taskName,
                                            const QString &processorId) const;

    /// @brief List registered processor IDs for a task.
    QStringList availableProcessors(const QString &taskName) const;

    /// @brief List all registered task names.
    QStringList availableTasks() const;

    /// @brief Self-registration helper (Essentia Registrar pattern).
    /// Place a static instance in the processor's .cpp file.
    template <typename ProcessorType>
    struct Registrar {
        Registrar(const QString &taskName, const QString &processorId) {
            TaskProcessorRegistry::instance().registerProcessor(
                taskName, processorId,
                [] { return std::make_unique<ProcessorType>(); });
        }
    };

private:
    TaskProcessorRegistry() = default;
    mutable std::mutex m_mutex;
    std::map<QString, std::map<QString, ProcessorFactory>> m_registry;
};

} // namespace dstools
