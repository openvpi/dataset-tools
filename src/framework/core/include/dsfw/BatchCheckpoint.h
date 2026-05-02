#pragma once
/// @file BatchCheckpoint.h
/// @brief Persistent checkpoint for resumable batch processing tasks.

#include <QString>
#include <QStringList>

namespace dstools {

/// @deprecated Use PipelineContext JSON persistence instead. See ADR-37.
class [[deprecated("Use PipelineContext JSON persistence instead")]] BatchCheckpoint {
public:
    /// @brief Load an existing checkpoint from disk.
    /// @param workingDir Working directory containing the checkpoint file.
    /// @param taskName Name identifying the batch task.
    /// @return Loaded checkpoint (may be empty if none exists).
    static BatchCheckpoint load(const QString &workingDir, const QString &taskName);

    BatchCheckpoint() = default;

    /// @brief Construct a new checkpoint for a task.
    /// @param workingDir Working directory for the checkpoint file.
    /// @param taskName Name identifying the batch task.
    BatchCheckpoint(const QString &workingDir, const QString &taskName);

    /// @brief Save the checkpoint to disk.
    void save();

    /// @brief Check whether a checkpoint file exists on disk.
    /// @return True if the checkpoint file exists.
    bool exists() const;

    /// @brief Record a file as successfully processed.
    /// @param file Path of the processed file.
    void addProcessedFile(const QString &file);

    /// @brief Record a file as failed with an error message.
    /// @param file Path of the failed file.
    /// @param error Error description.
    void addFailedFile(const QString &file, const QString &error);

    /// @brief Check whether a file has already been processed.
    /// @param file Path to check.
    /// @return True if the file was already processed.
    bool isFileProcessed(const QString &file) const;

    /// @brief Remove the checkpoint file from disk.
    void remove();

    /// @brief Get the task name.
    /// @return Task name.
    QString taskName() const { return m_taskName; }

    /// @brief Get the processor identifier.
    /// @return Processor ID string.
    QString processorId() const { return m_processorId; }

    /// @brief Get the task start time.
    /// @return Start time string.
    QString startTime() const { return m_startTime; }

    /// @brief Get the list of successfully processed files.
    /// @return Processed file paths.
    QStringList processedFiles() const { return m_processedFiles; }

    /// @brief Get the list of failed files.
    /// @return Failed file paths.
    QStringList failedFiles() const { return m_failedFiles; }

    /// @brief Set the processor identifier.
    /// @param id Processor ID string.
    void setProcessorId(const QString &id) { m_processorId = id; }

private:
    QString checkpointPath() const;

    QString m_workingDir;
    QString m_taskName;
    QString m_processorId;
    QString m_startTime;
    QStringList m_processedFiles;
    QStringList m_failedFiles;
};

} // namespace dstools
