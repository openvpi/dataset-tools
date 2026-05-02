#include <dsfw/TaskProcessorRegistry.h>

#include <QDir>
#include <QFileInfo>
#include <QSet>

namespace dstools {

TaskProcessorRegistry &TaskProcessorRegistry::instance() {
    static TaskProcessorRegistry s_instance;
    return s_instance;
}

void TaskProcessorRegistry::registerProcessor(const QString &taskName,
                                               const QString &processorId,
                                               ProcessorFactory factory) {
    std::lock_guard lock(m_mutex);
    m_registry[taskName][processorId] = std::move(factory);
}

std::unique_ptr<ITaskProcessor> TaskProcessorRegistry::create(
    const QString &taskName, const QString &processorId) const {
    std::lock_guard lock(m_mutex);
    auto taskIt = m_registry.find(taskName);
    if (taskIt == m_registry.end())
        return nullptr;
    auto procIt = taskIt->second.find(processorId);
    if (procIt == taskIt->second.end())
        return nullptr;
    return procIt->second();
}

QStringList TaskProcessorRegistry::availableProcessors(const QString &taskName) const {
    std::lock_guard lock(m_mutex);
    QStringList result;
    auto it = m_registry.find(taskName);
    if (it != m_registry.end())
        for (const auto &[id, _] : it->second)
            result.append(id);
    return result;
}

QStringList TaskProcessorRegistry::availableTasks() const {
    std::lock_guard lock(m_mutex);
    QStringList result;
    for (const auto &[name, _] : m_registry)
        result.append(name);
    return result;
}

} // namespace dstools
