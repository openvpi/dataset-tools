#include <dstools/ModelManager.h>

#include <QDateTime>

#include <algorithm>

namespace dstools {

ModelManager::ModelManager(QObject *parent) : QObject(parent) {
}

ModelManager::~ModelManager() {
    unloadAll();
}

void ModelManager::registerProvider(ModelType type, std::unique_ptr<IModelProvider> provider) {
    Entry entry;
    entry.provider = std::move(provider);
    m_entries.insert(type, std::move(entry));
}

IModelProvider *ModelManager::provider(ModelType type) const {
    auto it = m_entries.find(type);
    if (it == m_entries.end())
        return nullptr;
    return it->provider.get();
}

bool ModelManager::ensureLoaded(ModelType type, const QString &modelPath, int gpuIndex,
                                std::string &error) {
    auto it = m_entries.find(type);
    if (it == m_entries.end()) {
        error = "No provider registered for this model type";
        return false;
    }

    auto &entry = it.value();

    // Already loaded with same path and GPU index
    if (entry.provider->status() == ModelStatus::Ready && entry.lastPath == modelPath &&
        entry.lastGpuIndex == gpuIndex) {
        entry.lastUsedTimestamp = QDateTime::currentMSecsSinceEpoch();
        return true;
    }

    // Unload if currently loaded with different config
    if (entry.provider->status() == ModelStatus::Ready)
        entry.provider->unload();

    // Evict other models if needed
    evictIfNeeded(entry.provider->estimatedMemoryBytes());

    emit modelStatusChanged(type, ModelStatus::Loading);

    if (!entry.provider->load(modelPath, gpuIndex, error)) {
        emit modelStatusChanged(type, ModelStatus::Error);
        return false;
    }

    entry.lastPath = modelPath;
    entry.lastGpuIndex = gpuIndex;
    entry.lastUsedTimestamp = QDateTime::currentMSecsSinceEpoch();

    emit modelStatusChanged(type, ModelStatus::Ready);
    emit memoryUsageChanged(currentMemoryUsage());
    return true;
}

void ModelManager::unload(ModelType type) {
    auto it = m_entries.find(type);
    if (it == m_entries.end())
        return;

    auto &entry = it.value();
    if (entry.provider->status() == ModelStatus::Ready) {
        entry.provider->unload();
        emit modelStatusChanged(type, ModelStatus::Unloaded);
        emit memoryUsageChanged(currentMemoryUsage());
    }
}

void ModelManager::unloadAll() {
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        if (it->provider->status() == ModelStatus::Ready) {
            it->provider->unload();
            emit modelStatusChanged(it.key(), ModelStatus::Unloaded);
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
        if (it->provider->status() == ModelStatus::Ready)
            total += it->provider->estimatedMemoryBytes();
    }
    return total;
}

ModelStatus ModelManager::status(ModelType type) const {
    auto it = m_entries.find(type);
    if (it == m_entries.end())
        return ModelStatus::Unloaded;
    return it->provider->status();
}

QList<ModelType> ModelManager::loadedModels() const {
    QList<ModelType> result;
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        if (it->provider->status() == ModelStatus::Ready)
            result.append(it.key());
    }
    return result;
}

void ModelManager::evictIfNeeded(int64_t requiredBytes) {
    if (m_memoryLimit <= 0)
        return;

    while (currentMemoryUsage() + requiredBytes > m_memoryLimit) {
        // Find the ready provider with the oldest timestamp
        ModelType oldest = {};
        qint64 oldestTimestamp = std::numeric_limits<qint64>::max();
        bool found = false;

        for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
            if (it->provider->status() == ModelStatus::Ready &&
                it->lastUsedTimestamp < oldestTimestamp) {
                oldestTimestamp = it->lastUsedTimestamp;
                oldest = it.key();
                found = true;
            }
        }

        if (!found)
            break;

        unload(oldest);
    }
}

} // namespace dstools
