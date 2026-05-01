#include <dstools/ModelManager.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QThread>

#include <algorithm>

namespace dstools {

ModelManager::ModelManager(QObject *parent) : IModelManager(parent) {
}

ModelManager::~ModelManager() {
    unloadAll();
}

void ModelManager::registerProvider(ModelTypeId type, std::unique_ptr<IModelProvider> provider) {
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    Entry entry;
    entry.provider = std::move(provider);
    m_entries.emplace(type, std::move(entry));
}

IModelProvider *ModelManager::provider(ModelTypeId type) const {
    auto it = m_entries.find(type);
    if (it == m_entries.end())
        return nullptr;
    return it->second.provider.get();
}

Result<void> ModelManager::ensureLoaded(ModelTypeId type, const QString &modelPath, int gpuIndex) {
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    auto it = m_entries.find(type);
    if (it == m_entries.end()) {
        return Result<void>::Error("No provider registered for this model type");
    }

    auto &entry = it->second;
    if (entry.provider->status() == ModelStatus::Ready && entry.lastPath == modelPath &&
        entry.lastGpuIndex == gpuIndex) {
        entry.lastUsedTimestamp = QDateTime::currentMSecsSinceEpoch();
        return Result<void>::Ok();
    }

    if (entry.provider->status() == ModelStatus::Ready)
        entry.provider->unload();

    evictIfNeeded(entry.provider->estimatedMemoryBytes());

    emit modelStatusChanged(type, ModelStatus::Loading);

    auto loadResult = entry.provider->load(modelPath, gpuIndex);
    if (!loadResult) {
        emit modelStatusChanged(type, ModelStatus::Error);
        return loadResult;
    }

    entry.lastPath = modelPath;
    entry.lastGpuIndex = gpuIndex;
    entry.lastUsedTimestamp = QDateTime::currentMSecsSinceEpoch();

    emit modelStatusChanged(type, ModelStatus::Ready);
    emit memoryUsageChanged(currentMemoryUsage());
    return Result<void>::Ok();
}

void ModelManager::unload(ModelTypeId type) {
    Q_ASSERT(QThread::currentThread() == QCoreApplication::instance()->thread());
    auto it = m_entries.find(type);
    if (it == m_entries.end())
        return;

    auto &entry = it->second;
    if (entry.provider->status() == ModelStatus::Ready) {
        entry.provider->unload();
        emit modelStatusChanged(type, ModelStatus::Unloaded);
        emit memoryUsageChanged(currentMemoryUsage());
    }
}

void ModelManager::unloadAll() {
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        if (it->second.provider->status() == ModelStatus::Ready) {
            it->second.provider->unload();
            emit modelStatusChanged(it->first, ModelStatus::Unloaded);
        }
    }
    emit memoryUsageChanged(0);
}

void ModelManager::setMemoryLimit(int64_t bytes) {
    m_memoryLimit = bytes;
}

int64_t ModelManager::memoryLimit() const {
    return m_memoryLimit;
}

int64_t ModelManager::currentMemoryUsage() const {
    int64_t total = 0;
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        if (it->second.provider->status() == ModelStatus::Ready)
            total += it->second.provider->estimatedMemoryBytes();
    }
    return total;
}

ModelStatus ModelManager::status(ModelTypeId type) const {
    auto it = m_entries.find(type);
    if (it == m_entries.end())
        return ModelStatus::Unloaded;
    return it->second.provider->status();
}

QList<ModelTypeId> ModelManager::loadedModels() const {
    QList<ModelTypeId> result;
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        if (it->second.provider->status() == ModelStatus::Ready)
            result.append(it->first);
    }
    return result;
}

void ModelManager::evictIfNeeded(int64_t requiredBytes) {
    if (m_memoryLimit <= 0)
        return;

    while (currentMemoryUsage() + requiredBytes > m_memoryLimit) {
        ModelTypeId oldest;
        qint64 oldestTimestamp = std::numeric_limits<qint64>::max();
        bool found = false;

        for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
            if (it->second.provider->status() == ModelStatus::Ready &&
                it->second.lastUsedTimestamp < oldestTimestamp) {
                oldestTimestamp = it->second.lastUsedTimestamp;
                oldest = it->first;
                found = true;
            }
        }

        if (!found)
            break;

        unload(oldest);
    }
}

} // namespace dstools
