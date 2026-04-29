#include <dstools/AppSettings.h>
#include <dstools/JsonHelper.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QSaveFile>

namespace dstools {

AppSettings::AppSettings(const QString &appName, QObject *parent)
    : QObject(parent) {
    const QString configDir = QCoreApplication::applicationDirPath() + QStringLiteral("/config");
    QDir dir(configDir);
    if (!dir.exists())
        dir.mkpath(QStringLiteral("."));

    m_filePath = configDir + QStringLiteral("/") + appName + QStringLiteral(".json");
    loadFromDisk();
}

void AppSettings::loadFromDisk() {
    namespace fs = std::filesystem;
    const fs::path path = m_filePath.toStdWString();
    if (!fs::exists(path)) {
        m_data = nlohmann::json::object();
        return;
    }

    std::string error;
    m_data = JsonHelper::loadFile(path, error);
    if (!error.empty()) {
        qWarning() << "AppSettings:" << QString::fromStdString(error);
        m_data = nlohmann::json::object();
    }
    // loadFile may return an array (valid JSON but not our format)
    if (!m_data.is_object()) {
        qWarning() << "AppSettings: root is not a JSON object in" << m_filePath;
        m_data = nlohmann::json::object();
    }
}

bool AppSettings::saveToDisk() {
    // Use QSaveFile for Qt-native atomic writes (preferred in Qt context)
    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "AppSettings: cannot open" << m_filePath << "for writing";
        return false;
    }

    const std::string content = m_data.dump(4);
    file.write(content.c_str(), static_cast<qint64>(content.size()));
    file.write("\n", 1);

    if (!file.commit()) {
        qWarning() << "AppSettings: failed to commit (atomic write)" << m_filePath;
        return false;
    }
    return true;
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
