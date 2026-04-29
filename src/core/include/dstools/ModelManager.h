#pragma once

#include <dstools/IModelProvider.h>

#include <QObject>
#include <QMap>
#include <QString>

#include <memory>

namespace dstools {

class ModelManager : public QObject {
    Q_OBJECT
public:
    explicit ModelManager(QObject *parent = nullptr);
    ~ModelManager() override;

    // Registration
    void registerProvider(ModelType type, std::unique_ptr<IModelProvider> provider);
    IModelProvider *provider(ModelType type) const;

    // Lifecycle
    bool ensureLoaded(ModelType type, const QString &modelPath, int gpuIndex, std::string &error);
    void unload(ModelType type);
    void unloadAll();

    // Memory management
    void setMemoryLimit(int64_t bytes);
    int64_t memoryLimit() const;
    int64_t currentMemoryUsage() const;

    // Query
    ModelStatus status(ModelType type) const;
    QList<ModelType> loadedModels() const;

signals:
    void modelStatusChanged(ModelType type, ModelStatus status);
    void memoryUsageChanged(int64_t totalBytes);

private:
    struct Entry {
        std::unique_ptr<IModelProvider> provider;
        QString lastPath;
        int lastGpuIndex = -1;
        qint64 lastUsedTimestamp = 0;
    };

    QMap<ModelType, Entry> m_entries;
    int64_t m_memoryLimit = 0;

    void evictIfNeeded(int64_t requiredBytes);
};

} // namespace dstools
