#include <dsfw/AppPaths.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

namespace dsfw {

QString AppPaths::ensureDir(const QString &path) {
    QDir dir(path);
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));
    return path;
}

QString AppPaths::dataDir() {
    return ensureDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
}

QString AppPaths::configDir() {
    return ensureDir(dataDir() + QStringLiteral("/config"));
}

QString AppPaths::logDir() {
    return ensureDir(dataDir() + QStringLiteral("/logs"));
}

QString AppPaths::dumpDir() {
    return ensureDir(dataDir() + QStringLiteral("/crash_dumps"));
}

QString AppPaths::cacheDir() {
    return ensureDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
}

void AppPaths::migrateFromLegacyPaths() {
    const QString legacyBase = QCoreApplication::applicationDirPath();
    const QString newBase = dataDir();

    const QStringList subDirs = {
        QStringLiteral("/config"),
        QStringLiteral("/crash_dumps"),
        QStringLiteral("/logs"),
    };

    for (const auto &sub : subDirs) {
        QDir legacyDir(legacyBase + sub);
        if (!legacyDir.exists())
            continue;

        QDir newDir(newBase + sub);
        if (!newDir.exists())
            newDir.mkpath(QStringLiteral("."));

        const QFileInfoList entries = legacyDir.entryInfoList(
            QDir::Files | QDir::NoDotAndDotDot);
        for (const auto &fi : entries) {
            const QString dest = newDir.absoluteFilePath(fi.fileName());
            if (QFile::exists(dest))
                continue;
            if (QFile::copy(fi.absoluteFilePath(), dest)) {
                qDebug() << "Migrated:" << fi.absoluteFilePath() << "->" << dest;
            } else {
                qWarning() << "Failed to migrate:" << fi.absoluteFilePath();
            }
        }
    }
}

} // namespace dsfw
