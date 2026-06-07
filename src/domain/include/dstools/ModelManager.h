#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <dsfw/IModelProvider.h>
#include <dsfw/Result.h>
#include <map>
#include <memory>
#include <mutex>

namespace dstools {

    using namespace dsfw;

    struct TaskModelConfig;

    class ModelManager : public QObject {
        Q_OBJECT
    public:
        static ModelManager &instance();

        explicit ModelManager(QObject *parent = nullptr);
        ~ModelManager() override;

        IModelProvider *provider(ModelTypeId type) const;

        Result<void> ensureLoaded(ModelTypeId type, const QString &modelPath, int gpuIndex);
        void unload(ModelTypeId type);
        void unloadAll();
        void invalidateModel(const QString &taskKey);

        void registerProvider(ModelTypeId type, std::unique_ptr<IModelProvider> provider);

        Result<void> loadModel(const QString &taskKey, const TaskModelConfig &config, int gpuIndex);
        void unloadModel(const QString &taskKey);

        void setMemoryLimit(int64_t bytes);
        int64_t memoryLimit() const;
        int64_t currentMemoryUsage() const;

        ModelStatus status(ModelTypeId type) const;
        QList<ModelTypeId> loadedModels() const;

    signals:
        void modelStatusChanged(ModelTypeId type, ModelStatus status);
        void memoryUsageChanged(int64_t totalBytes);
        void modelInvalidated(const QString &taskKey);

    private:
        struct Entry {
            std::unique_ptr<IModelProvider> provider;
            QString lastPath;
            int lastGpuIndex = -1;
            qint64 lastUsedTimestamp = 0;
        };

        std::map<ModelTypeId, Entry> m_entries;
        mutable std::recursive_mutex m_entriesMutex;
        int64_t m_memoryLimit = 0;

        void evictIfNeeded(int64_t requiredBytes);
    };

} // namespace dstools