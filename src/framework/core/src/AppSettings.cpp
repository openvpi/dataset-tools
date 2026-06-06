#include <dsfw/AppSettings.h>
#include <dsfw/AppPaths.h>
#include <dsfw/AtomicFileWriter.h>
#include <dsfw/JsonHelper.h>
#include <dsfw/Log.h>
#include <dsfw/PathUtils.h>

#include <QDir>

#include <nlohmann/json.hpp>
#include <sstream>

namespace dsfw {

namespace {

    nlohmann::json toJson(const QString &v) {
        return v.toStdString();
    }
    nlohmann::json toJson(const std::string &v) {
        return v;
    }
    nlohmann::json toJson(int v) {
        return v;
    }
    nlohmann::json toJson(double v) {
        return v;
    }
    nlohmann::json toJson(float v) {
        return static_cast<double>(v);
    }
    nlohmann::json toJson(bool v) {
        return v;
    }

    void assignJson(nlohmann::json &root, const char *path, const nlohmann::json &value) {
        if (!path || path[0] == '\0')
            return;
        nlohmann::json *node = &root;
        std::string segment;
        std::istringstream ss(path);
        std::vector<std::string> segments;
        while (std::getline(ss, segment, '/')) {
            if (!segment.empty())
                segments.push_back(segment);
        }
        if (segments.empty())
            return;
        for (size_t i = 0; i + 1 < segments.size(); ++i) {
            auto &s = segments[i];
            if (!node->contains(s) || !(*node)[s].is_object())
                (*node)[s] = nlohmann::json::object();
            node = &(*node)[s];
        }
        (*node)[segments.back()] = value;
    }

} // anonymous namespace

class AppSettingsData {
public:
    nlohmann::json m_data;
    QString m_filePath;
    bool m_dirty = false;

    void loadFromDisk() {
        const std::filesystem::path path = m_filePath.toStdWString();
        if (!std::filesystem::exists(path)) {
            m_data = nlohmann::json::object();
            return;
        }

        auto loadResult = dsfw::JsonHelper::loadFile(path);
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

    bool saveToDisk() {
        std::string content = m_data.dump(4);
        content += '\n';
        auto result = dsfw::AtomicFileWriter::write(
            dsfw::PathUtils::toStdPath(m_filePath), content);
        if (!result) {
            DSFW_LOG_WARN("io", ("AppSettings: " + std::string(result.error())).c_str());
            return false;
        }
        return true;
    }

    void migrateIfNeeded() {
        constexpr int kCurrentVersion = 1;

        int storedVersion = 0;
        if (const auto *node = dsfw::JsonHelper::resolve(m_data, "General/settingsVersion")) {
            if (node->is_number_integer()) {
                storedVersion = node->get<int>();
            }
        }

        if (storedVersion >= kCurrentVersion)
            return;

        assignJson(m_data, "General/settingsVersion", nlohmann::json(kCurrentVersion));
    }

    QString getString(const char *path, const QString &defaultValue) const {
        if (const auto *node = dsfw::JsonHelper::resolve(m_data, path)) {
            if (node->is_string())
                return QString::fromStdString(node->get<std::string>());
        }
        return defaultValue;
    }

    int getInt(const char *path, int defaultValue) const {
        if (const auto *node = dsfw::JsonHelper::resolve(m_data, path)) {
            if (node->is_number_integer())
                return node->get<int>();
        }
        return defaultValue;
    }

    double getDouble(const char *path, double defaultValue) const {
        if (const auto *node = dsfw::JsonHelper::resolve(m_data, path)) {
            if (node->is_number())
                return node->get<double>();
        }
        return defaultValue;
    }

    float getFloat(const char *path, float defaultValue) const {
        if (const auto *node = dsfw::JsonHelper::resolve(m_data, path)) {
            if (node->is_number())
                return node->get<float>();
        }
        return defaultValue;
    }

    bool getBool(const char *path, bool defaultValue) const {
        if (const auto *node = dsfw::JsonHelper::resolve(m_data, path)) {
            if (node->is_boolean())
                return node->get<bool>();
        }
        return defaultValue;
    }

    std::string getStdString(const char *path, const std::string &defaultValue) const {
        if (const auto *node = dsfw::JsonHelper::resolve(m_data, path)) {
            if (node->is_string())
                return node->get<std::string>();
        }
        return defaultValue;
    }

    void setString(const char *path, const QString &value) {
        assignJson(m_data, path, toJson(value));
        m_dirty = true;
    }
    void setInt(const char *path, int value) {
        assignJson(m_data, path, toJson(value));
        m_dirty = true;
    }
    void setDouble(const char *path, double value) {
        assignJson(m_data, path, toJson(value));
        m_dirty = true;
    }
    void setFloat(const char *path, float value) {
        assignJson(m_data, path, toJson(value));
        m_dirty = true;
    }
    void setBool(const char *path, bool value) {
        assignJson(m_data, path, toJson(value));
        m_dirty = true;
    }
    void setStdString(const char *path, const std::string &value) {
        assignJson(m_data, path, toJson(value));
        m_dirty = true;
    }

    bool hasKey(const char *path) const {
        return dsfw::JsonHelper::resolve(m_data, path) != nullptr;
    }

    bool eraseKey(const char *path) {
        std::string segment;
        std::istringstream ss(path);
        std::vector<std::string> segments;
        while (std::getline(ss, segment, '/')) {
            if (!segment.empty())
                segments.push_back(segment);
        }
        if (segments.empty())
            return false;

        nlohmann::json *node = &m_data;
        for (size_t i = 0; i + 1 < segments.size(); ++i) {
            if (!node->contains(segments[i]))
                return false;
            node = &(*node)[segments[i]];
        }
        if (node->contains(segments.back())) {
            node->erase(segments.back());
            return true;
        }
        return false;
    }

    QString getRawString(const char *path, const QString &fallback) const {
        if (const auto *node = dsfw::JsonHelper::resolve(m_data, path))
            return QString::fromStdString(node->dump());
        return fallback;
    }

    void setRawString(const char *path, const QString &value) {
        auto j = nlohmann::json::parse(value.toStdString(), nullptr, false);
        if (!j.is_discarded()) {
            assignJson(m_data, path, j);
            m_dirty = true;
        }
    }
};

AppSettings::AppSettings(const QString &appName, QObject *parent)
    : QObject(parent), m_impl(std::make_unique<AppSettingsData>()) {
    const QString configDir = dsfw::AppPaths::configDir();
    m_impl->m_filePath = QDir(configDir).filePath(appName + QStringLiteral(".json"));
    m_impl->loadFromDisk();
    m_impl->migrateIfNeeded();
    m_impl->saveToDisk();
    m_impl->m_dirty = false;

    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(500);
    connect(m_saveTimer, &QTimer::timeout, this, [this]() {
        std::lock_guard lock(m_mutex);
        if (m_impl->m_dirty) {
            m_impl->saveToDisk();
            m_impl->m_dirty = false;
        }
    });
}

AppSettings::~AppSettings() {
    flush();
}

void AppSettings::reload() {
    std::lock_guard lock(m_mutex);
    m_impl->loadFromDisk();
}

void AppSettings::flush() {
    std::lock_guard lock(m_mutex);
    m_saveTimer->stop();
    m_impl->saveToDisk();
    m_impl->m_dirty = false;
}

QString AppSettings::filePath() const {
    return m_impl->m_filePath;
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

// ---- Bridge methods ----------------------------------------------------------

QString AppSettings::getValue(const char *path, const QString &defaultValue) const {
    return m_impl->getString(path, defaultValue);
}
int AppSettings::getValue(const char *path, int defaultValue) const {
    return m_impl->getInt(path, defaultValue);
}
double AppSettings::getValue(const char *path, double defaultValue) const {
    return m_impl->getDouble(path, defaultValue);
}
float AppSettings::getValue(const char *path, float defaultValue) const {
    return m_impl->getFloat(path, defaultValue);
}
bool AppSettings::getValue(const char *path, bool defaultValue) const {
    return m_impl->getBool(path, defaultValue);
}
std::string AppSettings::getValue(const char *path, const std::string &defaultValue) const {
    return m_impl->getStdString(path, defaultValue);
}

void AppSettings::setValue(const char *path, const QString &value) {
    m_impl->setString(path, value);
}
void AppSettings::setValue(const char *path, int value) {
    m_impl->setInt(path, value);
}
void AppSettings::setValue(const char *path, double value) {
    m_impl->setDouble(path, value);
}
void AppSettings::setValue(const char *path, float value) {
    m_impl->setFloat(path, value);
}
void AppSettings::setValue(const char *path, bool value) {
    m_impl->setBool(path, value);
}
void AppSettings::setValue(const char *path, const std::string &value) {
    m_impl->setStdString(path, value);
}

bool AppSettings::hasKey(const char *path) const {
    return m_impl->hasKey(path);
}
bool AppSettings::eraseKey(const char *path) {
    return m_impl->eraseKey(path);
}

QString AppSettings::getRawString(const char *path, const QString &fallback) const {
    std::lock_guard lock(m_mutex);
    return m_impl->getRawString(path, fallback);
}
void AppSettings::setRawString(const char *path, const QString &value) {
    std::lock_guard lock(m_mutex);
    m_impl->setRawString(path, value);
    QMetaObject::invokeMethod(m_saveTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
    emit keyChanged(QString::fromUtf8(path));
}

void AppSettings::loadFromDisk() {
    m_impl->loadFromDisk();
}
void AppSettings::doFlush() {
    m_impl->saveToDisk();
    m_impl->m_dirty = false;
}
void AppSettings::migrateIfNeeded() {
    m_impl->migrateIfNeeded();
}

} // namespace dsfw