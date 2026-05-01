#include <dsfw/TaskProcessorRegistry.h>
#include <dsfw/BatchCheckpoint.h>

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

// Default processBatch implementation
Result<BatchOutput> ITaskProcessor::processBatch(const BatchInput &input,
                                                 ProgressCallback progress) {
    QDir dir(input.workingDir);
    if (!dir.exists())
        return Err<BatchOutput>("Working directory does not exist");

    TaskSpec spec = taskSpec();
    BatchCheckpoint checkpoint = BatchCheckpoint::load(input.workingDir, spec.taskName);
    checkpoint.setProcessorId(processorId());

    static const QSet<QString> audioExts = {
        "wav", "mp3", "flac", "ogg", "m4a", "aac", "wma", "opus"
    };

    QStringList files;
    for (const QFileInfo &fi : dir.entryInfoList(QDir::Files)) {
        if (audioExts.contains(fi.suffix().toLower()))
            files.append(fi.fileName());
    }

    if (files.isEmpty())
        return Err<BatchOutput>("No audio files found in working directory");

    if (!checkpoint.exists())
        checkpoint.save();

    BatchOutput output;
    int current = 0;
    int total = files.size();

    for (const QString &fileName : files) {
        current++;
        if (checkpoint.isFileProcessed(fileName)) {
            output.processedCount++;
            output.processedFiles.append(fileName);
            if (progress)
                progress(current, total, fileName);
            continue;
        }

        TaskInput taskInput;
        taskInput.audioPath = dir.absoluteFilePath(fileName);
        taskInput.config = input.config;

        auto result = process(taskInput);
        if (result) {
            output.processedCount++;
            output.processedFiles.append(fileName);
            checkpoint.addProcessedFile(fileName);
        } else {
            output.failedCount++;
            output.failedFiles.append(fileName + QStringLiteral(": ") + QString::fromStdString(result.error()));
            checkpoint.addFailedFile(fileName, QString::fromStdString(result.error()));
        }

        if (progress)
            progress(current, total, fileName);
    }

    output.outputPath = input.workingDir;
    checkpoint.remove();
    return Ok(std::move(output));
}

} // namespace dstools
