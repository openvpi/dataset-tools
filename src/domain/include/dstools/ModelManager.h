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

    struct TaskModelConfig;

    class ModelManager : public QObject {
        Q_OBJECT
    public:
        static ModelManager &instance();

        explicit ModelManager(QObject *parent = nullptr);
        ~ModelManager() override;

        dsfw::IModelProvider *provider(dsfw::ModelTypeId type) const;

        dsfw::Result<void> ensureLoaded(dsfw::ModelTypeId type, const QString &modelPath, int gpuIndex);
        void unload(dsfw::ModelTypeId type);
        void unloadAll();
        void invalidateModel(const QString &taskKey);

        void registerProvider(dsfw::ModelTypeId type, std::unique_ptr<dsfw::IModelProvider> provider);

        dsfw::Result<void> loadModel(const QString &taskKey, const TaskModelConfig &config, int gpuIndex);
        void unloadModel(const QString &taskKey);

        void setMemoryLimit(int64_t bytes);
        int64_t memoryLimit() const;
        int64_t currentMemoryUsage() const;

        dsfw::ModelStatus status(dsfw::ModelTypeId type) const;
        QList<dsfw::ModelTypeId> loadedModels() const;

    signals:
        void modelStatusChanged(dsfw::ModelTypeId type, dsfw::ModelStatus status);
        void memoryUsageChanged(int64_t totalBytes);
        void modelInvalidated(const QString &taskKey);

    private:
        struct Entry {
            std::unique_ptr<dsfw::IModelProvider> provider;
            QString lastPath;
            int lastGpuIndex = -1;
            qint64 lastUsedTimestamp = 0;
        };

        std::map<dsfw::ModelTypeId, Entry> m_entries;
        mutable std::recursive_mutex m_entriesMutex;
        int64_t m_memoryLimit = 0;

        void evictIfNeeded(int64_t requiredBytes);
    };

} // namespace dstools