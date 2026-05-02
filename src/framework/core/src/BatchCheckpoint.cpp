#include <dsfw/BatchCheckpoint.h>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDateTime>

namespace dstools {

BatchCheckpoint BatchCheckpoint::load(const QString &workingDir, const QString &taskName) {
    BatchCheckpoint cp(workingDir, taskName);
    QString path = cp.checkpointPath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return cp;

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    file.close();
    if (error.error != QJsonParseError::NoError || !doc.isObject())
        return cp;

    QJsonObject obj = doc.object();
    cp.m_taskName = obj.value(QStringLiteral("taskName")).toString();
    cp.m_processorId = obj.value(QStringLiteral("processorId")).toString();
    cp.m_startTime = obj.value(QStringLiteral("startTime")).toString();

    QJsonArray procArr = obj.value(QStringLiteral("processedFiles")).toArray();
    for (const QJsonValue &v : procArr)
        cp.m_processedFiles.append(v.toString());

    QJsonArray failArr = obj.value(QStringLiteral("failedFiles")).toArray();
    for (const QJsonValue &v : failArr) {
        cp.m_failedFiles.append(
            v.toObject().value(QStringLiteral("file")).toString() +
            QStringLiteral(": ") +
            v.toObject().value(QStringLiteral("error")).toString());
    }

    return cp;
}

BatchCheckpoint::BatchCheckpoint(const QString &workingDir, const QString &taskName)
    : m_workingDir(workingDir), m_taskName(taskName),
      m_startTime(QDateTime::currentDateTime().toString(Qt::ISODate)) {}

QString BatchCheckpoint::checkpointPath() const {
    return m_workingDir + QStringLiteral("/dstemp/") + m_taskName + QStringLiteral(".checkpoint.json");
}

void BatchCheckpoint::save() {
    QString path = checkpointPath();
    QDir dir = QFileInfo(path).absoluteDir();
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));

    QJsonObject obj;
    obj[QStringLiteral("taskName")] = m_taskName;
    obj[QStringLiteral("processorId")] = m_processorId;
    obj[QStringLiteral("startTime")] = m_startTime;

    QJsonArray procArr;
    for (const QString &f : m_processedFiles)
        procArr.append(f);
    obj[QStringLiteral("processedFiles")] = procArr;

    QJsonArray failArr;
    for (const QString &entry : m_failedFiles) {
        int sepIdx = entry.indexOf(QStringLiteral(": "));
        QJsonObject failObj;
        if (sepIdx > 0) {
            failObj[QStringLiteral("file")] = entry.left(sepIdx);
            failObj[QStringLiteral("error")] = entry.mid(sepIdx + 2);
        } else {
            failObj[QStringLiteral("file")] = entry;
            failObj[QStringLiteral("error")] = QString();
        }
        failArr.append(failObj);
    }
    obj[QStringLiteral("failedFiles")] = failArr;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        file.close();
    } else {
        qWarning() << "BatchCheckpoint: failed to save checkpoint:" << path << file.errorString();
    }
}

bool BatchCheckpoint::exists() const {
    return QFile::exists(checkpointPath());
}

void BatchCheckpoint::addProcessedFile(const QString &file) {
    m_processedFiles.append(file);
    save();
}

void BatchCheckpoint::addFailedFile(const QString &file, const QString &error) {
    m_failedFiles.append(file + QStringLiteral(": ") + error);
    save();
}

bool BatchCheckpoint::isFileProcessed(const QString &file) const {
    return m_processedFiles.contains(file);
}

void BatchCheckpoint::remove() {
    QFile::remove(checkpointPath());
}

} // namespace dstools
