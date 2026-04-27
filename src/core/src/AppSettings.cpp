#include <dstools/AppSettings.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSaveFile>

#include <fstream>
#include <sstream>

namespace dstools {

AppSettings::AppSettings(const QString &appName, QObject *parent)
    : QObject(parent) {
    // Build path: <appDir>/config/<appName>.json
    const QString configDir = QCoreApplication::applicationDirPath() + QStringLiteral("/config");
    QDir dir(configDir);
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));

    m_filePath = configDir + QStringLiteral("/") + appName + QStringLiteral(".json");
    loadFromDisk();
}

void AppSettings::loadFromDisk() {
    QFile file(m_filePath);
    if (!file.exists()) {
        m_data = nlohmann::json::object();
        return;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "AppSettings: cannot open" << m_filePath << "for reading";
        m_data = nlohmann::json::object();
        return;
    }
    const QByteArray bytes = file.readAll();
    file.close();

    try {
        m_data = nlohmann::json::parse(bytes.constData(), bytes.constData() + bytes.size());
        if (!m_data.is_object()) {
            qWarning() << "AppSettings: root is not a JSON object in" << m_filePath;
            m_data = nlohmann::json::object();
        }
    } catch (const nlohmann::json::parse_error &e) {
        qWarning() << "AppSettings: parse error in" << m_filePath << ":" << e.what();
        m_data = nlohmann::json::object();
    }
}

void AppSettings::saveToDisk() {
    // Use QSaveFile for atomic writes (write to temp, then rename).
    // This prevents data loss if the app crashes mid-write.
    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "AppSettings: cannot open" << m_filePath << "for writing";
        return;
    }

    const std::string content = m_data.dump(4);  // pretty-print with 4-space indent
    file.write(content.c_str(), static_cast<qint64>(content.size()));
    file.write("\n", 1);

    if (!file.commit()) {
        qWarning() << "AppSettings: failed to commit (atomic write)" << m_filePath;
    }
}

void AppSettings::reload() {
    std::lock_guard lock(m_mutex);
    loadFromDisk();
}

void AppSettings::flush() {
    std::lock_guard lock(m_mutex);
    saveToDisk();
}

void AppSettings::removeObserver(int id) {
    std::lock_guard lock(m_mutex);
    for (auto &[path, entries] : m_observers) {
        entries.erase(
            std::remove_if(entries.begin(), entries.end(),
                           [id](const ObserverEntry &e) { return e.id == id; }),
            entries.end());
    }
}

} // namespace dstools
