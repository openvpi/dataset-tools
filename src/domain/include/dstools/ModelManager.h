#pragma once

/// @file ModelManager.h
/// @brief Multi-model lifecycle manager with memory limits and LRU eviction.

#include <dsfw/IModelManager.h>

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
class ModelManager : public IModelManager {
    Q_OBJECT
public:
    /// @brief Construct a model manager.
    /// @param parent Optional QObject parent.
    explicit ModelManager(QObject *parent = nullptr);
    ~ModelManager() override;

    void registerProvider(ModelTypeId type, std::unique_ptr<IModelProvider> provider) override;
    IModelProvider *provider(ModelTypeId type) const override;

    Result<void> ensureLoaded(ModelTypeId type, const QString &modelPath, int gpuIndex) override;
    void unload(ModelTypeId type) override;
    void unloadAll() override;

    void setMemoryLimit(int64_t bytes) override;
    int64_t memoryLimit() const override;
    int64_t currentMemoryUsage() const override;

    ModelStatus status(ModelTypeId type) const override;
    QList<ModelTypeId> loadedModels() const override;

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

} // namespace dstools
