#pragma once

/// @file IModelManager.h
/// @brief Abstract interface for multi-model lifecycle management.

#include <dsfw/IModelProvider.h>

#include <QList>
#include <QObject>
#include <QString>

#include <memory>

namespace dstools {

/// @brief Abstract interface for managing multiple IModelProvider instances.
///
/// Provides model registration, loading with memory-aware eviction, and status queries.
/// Implementations must emit signals when model status or memory usage changes.
class IModelManager : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;
    ~IModelManager() override = default;

    /// @brief Get the provider for a model type, or nullptr if not registered.
    virtual IModelProvider *provider(ModelTypeId type) const = 0;

    /// @brief Ensure a model is loaded, loading it if necessary.
    virtual Result<void> ensureLoaded(ModelTypeId type, const QString &modelPath, int gpuIndex) = 0;
    /// @brief Unload a specific model.
    virtual void unload(ModelTypeId type) = 0;
    /// @brief Unload all models.
    virtual void unloadAll() = 0;

signals:
    /// @brief Emitted when a model's status changes.
    void modelStatusChanged(ModelTypeId type, ModelStatus status);
    /// @brief Emitted when total memory usage changes.
    void memoryUsageChanged(int64_t totalBytes);
};

} // namespace dstools
