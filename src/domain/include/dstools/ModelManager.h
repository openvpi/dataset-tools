#pragma once

#include <dsfw/IModelManager.h>

#include <map>
#include <memory>

namespace dstools {

struct TaskModelConfig;

class ModelManager : public IModelManager {
    Q_OBJECT
public:
    explicit ModelManager(QObject *parent = nullptr);
    ~ModelManager() override;

    void registerProvider(ModelTypeId type, std::unique_ptr<IModelProvider> provider);
    IModelProvider *provider(ModelTypeId type) const override;

    Result<void> ensureLoaded(ModelTypeId type, const QString &modelPath, int gpuIndex) override;
    void unload(ModelTypeId type) override;
    void unloadAll() override;

    void setMemoryLimit(int64_t bytes);
    int64_t memoryLimit() const;
    int64_t currentMemoryUsage() const;

    ModelStatus status(ModelTypeId type) const;
    QList<ModelTypeId> loadedModels() const;

    Result<void> loadModel(const QString &taskKey, const TaskModelConfig &config, int gpuIndex);
    void unloadModel(const QString &taskKey);

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
