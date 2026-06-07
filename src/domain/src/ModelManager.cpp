#include <dstools/ModelManager.h>
#include <dstools/DsProject.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QThread>

#include <algorithm>
#include <atomic>
#include <mutex>

namespace dstools {

using namespace dsfw;

static ModelTypeId taskKeyToTypeId(const QString &taskKey) {
    return registerModelType(taskKey.toStdString());
}

static std::atomic<ModelManager *> s_instance{nullptr};

ModelManager &ModelManager::instance() {
    auto *p = s_instance.load(std::memory_order_acquire);
    if (!p) {
        qFatal("ModelManager::instance() called before construction or after destruction");
    }
    return *p;
}

ModelManager::ModelManager(QObject *parent) : QObject(parent) {
    auto *prev = s_instance.load(std::memory_order_acquire);
    if (prev) {
        qCritical() << "ModelManager constructed more than once; previous instance at" << prev << "will be orphaned";
    }
    s_instance.store(this, std::memory_order_release);
}

ModelManager::~ModelManager() {
    unloadAll();
    s_instance.store(nullptr, std::memory_order_release);
}

void ModelManager::registerProvider(ModelTypeId type, std::unique_ptr<IModelProvider> provider) {
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        qCritical() << "ModelManager::registerProvider called from non-main thread";
        return;
    }
    Entry entry;
    entry.provider = std::move(provider);
    std::lock_guard lock(m_entriesMutex);
    m_entries.emplace(type, std::move(entry));
}

IModelProvider *ModelManager::provider(ModelTypeId type) const {
    std::lock_guard lock(m_entriesMutex);
    auto it = m_entries.find(type);
    if (it == m_entries.end())
        return nullptr;
    return it->second.provider.get();
}

Result<void> ModelManager::ensureLoaded(ModelTypeId type, const QString &modelPath, int gpuIndex) {
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        qCritical() << "ModelManager::ensureLoaded called from non-main thread";
        return Result<void>::Error("ensureLoaded must be called from the main thread");
    }
    std::lock_guard lock(m_entriesMutex);
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
    if (QThread::currentThread() != QCoreApplication::instance()->thread()) {
        qCritical() << "ModelManager::unload called from non-main thread";
        return;
    }
    std::lock_guard lock(m_entriesMutex);
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
    std::lock_guard lock(m_entriesMutex);
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
    std::lock_guard lock(m_entriesMutex);
    int64_t total = 0;
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        if (it->second.provider->status() == ModelStatus::Ready)
            total += it->second.provider->estimatedMemoryBytes();
    }
    return total;
}

ModelStatus ModelManager::status(ModelTypeId type) const {
    std::lock_guard lock(m_entriesMutex);
    auto it = m_entries.find(type);
    if (it == m_entries.end())
        return ModelStatus::Unloaded;
    return it->second.provider->status();
}

QList<ModelTypeId> ModelManager::loadedModels() const {
    std::lock_guard lock(m_entriesMutex);
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

Result<void> ModelManager::loadModel(const QString &taskKey, const TaskModelConfig &config, int gpuIndex) {
    if (config.modelPath.isEmpty())
        return Result<void>::Error(("No model path configured for task: " + taskKey).toStdString());

    int effectiveGpuIndex = -1;
    if (config.provider == QStringLiteral("dml") || config.provider == QStringLiteral("cuda"))
        effectiveGpuIndex = config.deviceId >= 0 ? config.deviceId : gpuIndex;

    auto typeId = taskKeyToTypeId(taskKey);
    return ensureLoaded(typeId, config.modelPath, effectiveGpuIndex);
}

void ModelManager::unloadModel(const QString &taskKey) {
    auto typeId = taskKeyToTypeId(taskKey);
    unload(typeId);
}

void ModelManager::invalidateModel(const QString &taskKey) {
    auto typeId = taskKeyToTypeId(taskKey);
    emit modelInvalidated(taskKey);
    unload(typeId);
}

} // namespace dstools