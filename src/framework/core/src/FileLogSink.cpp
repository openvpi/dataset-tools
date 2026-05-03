#include <dsfw/FileLogSink.h>
#include <dsfw/AppPaths.h>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QMutexLocker>

namespace dsfw {

dstools::LogSink createFileLogSink(const QString &logDir, const QString &appName) {
    QDir dir(logDir);
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));

    QString fileName = logDir + QStringLiteral("/") + appName + QStringLiteral("_")
        + QDate::currentDate().toString(QStringLiteral("yyyyMMdd")) + QStringLiteral(".log");

    auto *file = new QFile(fileName);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        delete file;
        return [](const dstools::LogEntry &) {};
    }

    auto *mutex = new QMutex();

    return [file, mutex](const dstools::LogEntry &entry) {
        QMutexLocker locker(mutex);
        if (!file->isOpen()) return;

        const char *levelStr = "TRACE";
        switch (entry.level) {
            case dstools::LogLevel::Debug: levelStr = "DEBUG"; break;
            case dstools::LogLevel::Info: levelStr = "INFO"; break;
            case dstools::LogLevel::Warning: levelStr = "WARN"; break;
            case dstools::LogLevel::Error: levelStr = "ERROR"; break;
            case dstools::LogLevel::Fatal: levelStr = "FATAL"; break;
            default: break;
        }

        QDateTime dt = QDateTime::fromMSecsSinceEpoch(entry.timestampMs);
        QTextStream stream(file);
        stream << dt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))
               << QStringLiteral(" [") << levelStr << QStringLiteral("] ")
               << QString::fromStdString(entry.category)
               << QStringLiteral(": ")
               << QString::fromStdString(entry.message)
               << QStringLiteral("\n");
        stream.flush();
    };
}

void cleanOldLogFiles(const QString &logDir, int maxAgeDays) {
    QDir dir(logDir);
    if (!dir.exists()) return;

    QDate cutoff = QDate::currentDate().addDays(-maxAgeDays);

    for (const QFileInfo &fi : dir.entryInfoList(QStringList() << QStringLiteral("*.log"), QDir::Files)) {
        if (fi.lastModified().date() < cutoff) {
            QFile::remove(fi.absoluteFilePath());
        }
    }
}

} // namespace dsfw
