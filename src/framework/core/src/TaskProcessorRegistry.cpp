#include <dsfw/TaskProcessorRegistry.h>
#include <dsfw/PassthroughProcessor.h>

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QtCore/QDebug>

namespace dsfw {

TaskProcessorRegistry &TaskProcessorRegistry::instance() {
    static TaskProcessorRegistry s_instance;
    return s_instance;
}

void TaskProcessorRegistry::registerProcessor(const QString &taskName,
                                               const QString &processorId,
                                               ProcessorFactory factory) {
    auto temp = factory();
    if (!temp) {
        qCritical() << "TaskProcessorRegistry:" << processorId
                    << "factory returned null for task" << taskName;
        return;
    }
    auto version = temp->interfaceVersion();
    if (version != ITaskProcessor::kInterfaceVersion) {
        qWarning() << "TaskProcessorRegistry:" << processorId
                   << "reports interface version" << version
                   << "(expected" << ITaskProcessor::kInterfaceVersion << ")";
    }
    TaskSpec spec = temp->taskSpec();
    temp.reset();

    std::lock_guard lock(m_mutex);

    auto baselineIt = m_taskSpecBaselines.find(taskName);
    if (baselineIt != m_taskSpecBaselines.end()) {
        const auto &baseline = baselineIt->second;

        if (spec.inputs.size() != baseline.inputs.size()) {
            qCritical() << "TaskProcessorRegistry:" << processorId
                        << "input slot count mismatch for task" << taskName
                        << "- expected" << baseline.inputs.size() << "got" << spec.inputs.size()
                        << "- refusing registration";
            return;
        }
        if (spec.outputs.size() != baseline.outputs.size()) {
            qCritical() << "TaskProcessorRegistry:" << processorId
                        << "output slot count mismatch for task" << taskName
                        << "- expected" << baseline.outputs.size() << "got" << spec.outputs.size()
                        << "- refusing registration";
            return;
        }
        for (size_t i = 0; i < spec.inputs.size(); ++i) {
            if (spec.inputs[i].name != baseline.inputs[i].name
                || spec.inputs[i].category != baseline.inputs[i].category) {
                qCritical() << "TaskProcessorRegistry:" << processorId
                            << "input slot" << i << "mismatch for task" << taskName
                            << "- expected (" << baseline.inputs[i].name << "," << baseline.inputs[i].category << ")"
                            << "got (" << spec.inputs[i].name << "," << spec.inputs[i].category << ")"
                            << "- refusing registration";
                return;
            }
        }
        for (size_t i = 0; i < spec.outputs.size(); ++i) {
            if (spec.outputs[i].name != baseline.outputs[i].name
                || spec.outputs[i].category != baseline.outputs[i].category) {
                qCritical() << "TaskProcessorRegistry:" << processorId
                            << "output slot" << i << "mismatch for task" << taskName
                            << "- expected (" << baseline.outputs[i].name << "," << baseline.outputs[i].category << ")"
                            << "got (" << spec.outputs[i].name << "," << spec.outputs[i].category << ")"
                            << "- refusing registration";
                return;
            }
        }
    } else {
        m_taskSpecBaselines[taskName] = spec;
    }

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

QStringList TaskProcessorRegistry::availableTasks() const noexcept {
    std::lock_guard lock(m_mutex);
    QStringList result;
    for (const auto &[name, _] : m_registry)
        result.append(name);
    return result;
}

} // namespace dsfw

// ─── PassthroughProcessor implementation ──────────────────────────────────────

namespace dsfw {

QString PassthroughProcessor::processorId() const {
    return QStringLiteral("passthrough");
}

QString PassthroughProcessor::displayName() const {
    return QStringLiteral("Passthrough");
}

TaskSpec PassthroughProcessor::taskSpec() const {
    return {QStringLiteral("passthrough"), {}, {}};
}

Result<void> PassthroughProcessor::initialize(dstools::ModelManager &, const ProcessorConfig &) {
    return Result<void>::Ok();
}

void PassthroughProcessor::release() {
}

Result<TaskOutput> PassthroughProcessor::process(const TaskInput &input) {
    TaskOutput output;
    output.layers = input.layers;
    return Result<TaskOutput>::Ok(std::move(output));
}

#ifndef NDEBUG
static TaskProcessorRegistry::Registrar<PassthroughProcessor> s_passthroughReg(
    QStringLiteral("passthrough"), QStringLiteral("passthrough"));
#endif

} // namespace dsfw
