#include <dsfw/CrashSafeGuard.h>
#include <dsfw/Log.h>

#include <QFile>
#include <QDir>
#include <QDateTime>

namespace dsfw {

QString CrashSafeGuard::m_markerPath;
bool CrashSafeGuard::m_previousCrash = false;

void CrashSafeGuard::markStartup(const QString &markerDir) {
    m_markerPath = markerDir + QStringLiteral("/session_running");
    m_previousCrash = QFile::exists(m_markerPath);

    if (m_previousCrash) {
        DSFW_LOG_WARN("app", "Previous session marker found - abnormal exit detected");
    }

    QDir().mkpath(markerDir);

    QFile marker(m_markerPath);
    if (marker.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QByteArray content = QDateTime::currentDateTime()
            .toString(Qt::ISODate).toUtf8();
        marker.write(content);
        marker.flush();
        DSFW_LOG_DEBUG("app", ("Session marker written to: " + m_markerPath).toStdString().c_str());
    } else {
        DSFW_LOG_WARN("app", ("Failed to write session marker: " + m_markerPath).toStdString().c_str());
    }
}

void CrashSafeGuard::markCleanExit() {
    if (!m_markerPath.isEmpty() && QFile::exists(m_markerPath)) {
        QFile::remove(m_markerPath);
        DSFW_LOG_DEBUG("app", "Session marker removed on clean exit");
    }
}

bool CrashSafeGuard::wasPreviousCrash() {
    return m_previousCrash;
}

} // namespace dsfw
