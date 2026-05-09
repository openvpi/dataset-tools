#include <dsfw/AppSettings.h>
#include <dsfw/AppPaths.h>
#include <dsfw/JsonHelper.h>
#include <dsfw/Log.h>

#include <QSaveFile>

namespace dstools {

AppSettings::AppSettings(const QString &appName, QObject *parent)
    : QObject(parent) {
    const QString configDir = dsfw::AppPaths::configDir();
    m_filePath = configDir + QStringLiteral("/") + appName + QStringLiteral(".json");
    loadFromDisk();
    migrateIfNeeded();
    saveToDisk();

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

void AppSettings::migrateIfNeeded() {
    // Current settings format version
    constexpr int kCurrentVersion = 1;

    // Check stored version — if absent, it's version 0 (pre-versioning)
    int storedVersion = 0;
    if (const auto *node = JsonHelper::resolve(m_data, "General/settingsVersion")) {
        if (node->is_number_integer()) {
            storedVersion = node->get<int>();
        }
    }

    if (storedVersion >= kCurrentVersion)
        return;

    if (storedVersion < 1) {
        // Migration from v0 to v1: previous settings format is the same,
        // just write the version tag for future compatibility.
    }

    // Write current version
    detail::assign(m_data, "General/settingsVersion", nlohmann::json(kCurrentVersion));
}

void AppSettings::saveRaw(const char *path, const nlohmann::json &value) {
    std::lock_guard lock(m_mutex);
    detail::assign(m_data, path, value);
    m_dirty = true;
    QMetaObject::invokeMethod(m_saveTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
    emit keyChanged(QString::fromUtf8(path));
}

nlohmann::json AppSettings::getRaw(const char *path, const nlohmann::json &fallback) const {
    std::lock_guard lock(m_mutex);
    if (const auto *node = JsonHelper::resolve(m_data, path))
        return *node;
    return fallback;
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
        DSFW_LOG_WARN("app", ("AppSettings: " + loadResult.error()).c_str());
        m_data = nlohmann::json::object();
    } else {
        m_data = std::move(loadResult.value());
    }
    if (!m_data.is_object()) {
        DSFW_LOG_WARN("app", ("AppSettings: root is not a JSON object in " + m_filePath.toStdString()).c_str());
        m_data = nlohmann::json::object();
    }
}

bool AppSettings::saveToDisk() {
    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        DSFW_LOG_WARN("io", ("AppSettings: cannot open " + m_filePath.toStdString() + " for writing").c_str());
        return false;
    }

    const std::string content = m_data.dump(4);
    file.write(content.c_str(), static_cast<qint64>(content.size()));
    file.write("\n", 1);

    if (!file.commit()) {
        DSFW_LOG_WARN("io", ("AppSettings: failed to commit (atomic write) " + m_filePath.toStdString()).c_str());
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
