#include <dsfw/AppSettings.h>
#include <dsfw/AppPaths.h>
#include <dsfw/JsonHelper.h>

#include <QDebug>
#include <QSaveFile>

namespace dstools {

AppSettings::AppSettings(const QString &appName, QObject *parent)
    : QObject(parent) {
    const QString configDir = dsfw::AppPaths::configDir();
    m_filePath = configDir + QStringLiteral("/") + appName + QStringLiteral(".json");
    loadFromDisk();

    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(500);
    connect(m_saveTimer, &QTimer::timeout, this, [this]() {
        std::lock_guard lock(m_mutex);
        if (m_dirty) {
            saveToDisk();
            m_dirty = false;
        }
    });
}

void AppSettings::loadFromDisk() {
    namespace fs = std::filesystem;
    const fs::path path = m_filePath.toStdWString();
    if (!fs::exists(path)) {
        m_data = nlohmann::json::object();
        return;
    }

    auto loadResult = JsonHelper::loadFile(path);
    if (!loadResult) {
        qWarning() << "AppSettings:" << QString::fromStdString(loadResult.error());
        m_data = nlohmann::json::object();
    } else {
        m_data = std::move(loadResult.value());
    }
    if (!m_data.is_object()) {
        qWarning() << "AppSettings: root is not a JSON object in" << m_filePath;
        m_data = nlohmann::json::object();
    }
}

bool AppSettings::saveToDisk() {
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

AppSettings::~AppSettings() {
    flush();
}

void AppSettings::flush() {
    std::lock_guard lock(m_mutex);
    m_saveTimer->stop();
    saveToDisk();
    m_dirty = false;
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
