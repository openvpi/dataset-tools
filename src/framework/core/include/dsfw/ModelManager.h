#pragma once

/// @file ModelManager.h
/// @brief Multi-model lifecycle manager with memory limits and LRU eviction.

#include <dsfw/IModelProvider.h>

#include <QObject>
#include <QString>

#include <map>
#include <memory>

namespace dstools {

/// @brief Manages multiple IModelProvider instances with memory-aware LRU eviction.
///
/// Register model providers by type, then call ensureLoaded() to load on demand.
/// When memory usage exceeds the configured limit, the least-recently-used model
/// is evicted automatically.
///
/// @note All methods must be called from the main thread.
class ModelManager : public QObject {
    Q_OBJECT
public:
    /// @brief Construct a model manager.
    /// @param parent Optional QObject parent.
    explicit ModelManager(QObject *parent = nullptr);
    ~ModelManager() override;

    /// @brief Register a model provider for the given type.
    /// @param type Model type identifier.
    /// @param provider Ownership is transferred to the manager.
    void registerProvider(ModelTypeId type, std::unique_ptr<IModelProvider> provider);
    /// @brief Get the provider for a model type, or nullptr if not registered.
    /// @param type Model type identifier.
    /// @return Raw pointer to the provider (owned by this manager).
    IModelProvider *provider(ModelTypeId type) const;

    /// @brief Ensure a model is loaded, loading it if necessary.
    /// @param type Model type identifier.
    /// @param modelPath Path to the model directory or file.
    /// @param gpuIndex GPU device index (-1 for CPU).
    /// @return Success or error.
    /// @note May evict other models to stay within the memory limit.
    Result<void> ensureLoaded(ModelTypeId type, const QString &modelPath, int gpuIndex);
    /// @brief Unload a specific model.
    /// @param type Model type to unload.
    void unload(ModelTypeId type);
    /// @brief Unload all models.
    void unloadAll();

    /// @brief Set the maximum total memory usage in bytes (0 = unlimited).
    /// @param bytes Memory limit in bytes.
    void setMemoryLimit(int64_t bytes);
    /// @brief Return the configured memory limit.
    /// @return Memory limit in bytes (0 = unlimited).
    int64_t memoryLimit() const;
    /// @brief Return the total estimated memory usage of all loaded models.
    /// @return Total bytes used.
    int64_t currentMemoryUsage() const;

    /// @brief Return the status of a specific model type.
    /// @param type Model type identifier.
    /// @return Model status, or Unloaded if not registered.
    ModelStatus status(ModelTypeId type) const;
    /// @brief Return the list of currently loaded model types.
    /// @return List of ModelTypeId values with Ready status.
    QList<ModelTypeId> loadedModels() const;

signals:
    /// @brief Emitted when a model's status changes.
    /// @param type Model type whose status changed.
    /// @param status New status value.
    void modelStatusChanged(ModelTypeId type, ModelStatus status);
    /// @brief Emitted when total memory usage changes.
    /// @param totalBytes New total memory usage in bytes.
    void memoryUsageChanged(int64_t totalBytes);

private:
    struct Entry {
        std::unique_ptr<IModelProvider> provider;
        QString lastPath;
        int lastGpuIndex = -1;
        qint64 lastUsedTimestamp = 0;
    };

    std::map<ModelTypeId, Entry> m_entries;
    int64_t m_memoryLimit = 0;

    void evictIfNeeded(int64_t requiredBytes);
};

}
