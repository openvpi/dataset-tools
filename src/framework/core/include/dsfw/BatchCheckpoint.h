#pragma once

#include <QString>
#include <QStringList>

namespace dstools {

class BatchCheckpoint {
public:
    static BatchCheckpoint load(const QString &workingDir, const QString &taskName);

    BatchCheckpoint() = default;
    BatchCheckpoint(const QString &workingDir, const QString &taskName);

    void save();
    bool exists() const;
    void addProcessedFile(const QString &file);
    void addFailedFile(const QString &file, const QString &error);
    bool isFileProcessed(const QString &file) const;
    void remove();

    QString taskName() const { return m_taskName; }
    QString processorId() const { return m_processorId; }
    QString startTime() const { return m_startTime; }
    QStringList processedFiles() const { return m_processedFiles; }
    QStringList failedFiles() const { return m_failedFiles; }
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
