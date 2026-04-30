#pragma once

/// @file AppSettings.h
/// @brief Type-safe, reactive, JSON-backed persistent settings system.

#include <QCoreApplication>
#include <QDir>
#include <QKeySequence>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariant>

#include <dsfw/JsonHelper.h>
#include <nlohmann/json.hpp>

#include <functional>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace dstools {

template <typename T>
struct SettingsKey {
    const char *path;
    T defaultValue;

    SettingsKey(const char *path, T defaultValue)
        : path(path), defaultValue(std::move(defaultValue)) {}
};

namespace detail {

    inline nlohmann::json toJson(const QString &v) { return v.toStdString(); }
    inline nlohmann::json toJson(const std::string &v) { return v; }
    inline nlohmann::json toJson(int v) { return v; }
    inline nlohmann::json toJson(double v) { return v; }
    inline nlohmann::json toJson(float v) { return static_cast<double>(v); }
    inline nlohmann::json toJson(bool v) { return v; }
    inline nlohmann::json toJson(const QKeySequence &v) { return v.toString().toStdString(); }

    template <typename T>
    T fromJson(const nlohmann::json &j, const T &fallback);

    template <>
    inline QString fromJson<QString>(const nlohmann::json &j, const QString &fallback) {
        if (j.is_string()) return QString::fromStdString(j.get<std::string>());
        return fallback;
    }

    template <>
    inline std::string fromJson<std::string>(const nlohmann::json &j, const std::string &fallback) {
        if (j.is_string()) return j.get<std::string>();
        return fallback;
    }

    template <>
    inline int fromJson<int>(const nlohmann::json &j, const int &fallback) {
        if (j.is_number_integer()) return j.get<int>();
        return fallback;
    }

    template <>
    inline double fromJson<double>(const nlohmann::json &j, const double &fallback) {
        if (j.is_number()) return j.get<double>();
        return fallback;
    }

    template <>
    inline float fromJson<float>(const nlohmann::json &j, const float &fallback) {
        if (j.is_number()) return j.get<float>();
        return fallback;
    }

    template <>
    inline bool fromJson<bool>(const nlohmann::json &j, const bool &fallback) {
        if (j.is_boolean()) return j.get<bool>();
        return fallback;
    }

    template <>
    inline QKeySequence fromJson<QKeySequence>(const nlohmann::json &j, const QKeySequence &fallback) {
        if (j.is_string()) return QKeySequence(QString::fromStdString(j.get<std::string>()));
        return fallback;
    }

    inline void assign(nlohmann::json &root, const char *path, const nlohmann::json &value) {
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

} // namespace detail

class AppSettings : public QObject {
    Q_OBJECT

public:
    explicit AppSettings(const QString &appName, QObject *parent = nullptr);
    ~AppSettings() override;

    template <typename T>
    T get(const SettingsKey<T> &key) const {
        std::lock_guard lock(m_mutex);
        if (const auto *node = JsonHelper::resolve(m_data, key.path))
            return detail::fromJson<T>(*node, key.defaultValue);
        return key.defaultValue;
    }

    template <typename T>
    void set(const SettingsKey<T> &key, const T &value) {
        bool changed = false;
        {
            std::lock_guard lock(m_mutex);
            T current = get_unlocked(key);
            if (current != value) {
                detail::assign(m_data, key.path, detail::toJson(value));
                m_dirty = true;
                QMetaObject::invokeMethod(m_saveTimer, qOverload<>(&QTimer::start),
                                          Qt::QueuedConnection);
                changed = true;
            }
        }
        if (changed) {
            emit keyChanged(QString::fromUtf8(key.path));
            notifyObservers(key.path, value);
        }
    }

    template <typename T>
    int observe(const SettingsKey<T> &key, std::function<void(const T &)> callback,
                QObject *context = nullptr) {
        std::lock_guard lock(m_mutex);
        int id = m_nextObserverId++;
        m_observers[std::string(key.path)].push_back({id, [cb = std::move(callback)](const void *ptr) {
            cb(*static_cast<const T *>(ptr));
        }});
        if (context) {
            connect(context, &QObject::destroyed, this, [this, id]() {
                removeObserver(id);
            });
        }
        return id;
    }

    void removeObserver(int id);

    QKeySequence shortcut(const SettingsKey<QString> &key) const {
        return QKeySequence(get(key));
    }

    void setShortcut(const SettingsKey<QString> &key, const QKeySequence &seq) {
        set(key, seq.toString());
    }

    template <typename T>
    bool contains(const SettingsKey<T> &key) const {
        std::lock_guard lock(m_mutex);
        return JsonHelper::resolve(m_data, key.path) != nullptr;
    }

    template <typename T>
    void remove(const SettingsKey<T> &key) {
        bool removed = false;
        {
            std::lock_guard lock(m_mutex);
            std::string segment;
            std::istringstream ss(key.path);
            std::vector<std::string> segments;
            while (std::getline(ss, segment, '/')) {
                if (!segment.empty())
                    segments.push_back(segment);
            }
            if (segments.empty())
                return;

            nlohmann::json *node = &m_data;
            for (size_t i = 0; i + 1 < segments.size(); ++i) {
                if (!node->contains(segments[i])) return;
                node = &(*node)[segments[i]];
            }
            if (node->contains(segments.back())) {
                node->erase(segments.back());
                saveToDisk();
                removed = true;
            }
        }
        if (removed) {
            emit keyChanged(QString::fromUtf8(key.path));
            notifyObservers(key.path, key.defaultValue);
        }
    }

    void reload();
    void flush();
    QString filePath() const { return m_filePath; }

signals:
    void keyChanged(const QString &keyPath);

private:
    struct ObserverEntry {
        int id;
        std::function<void(const void *)> callback;
    };

    template <typename T>
    T get_unlocked(const SettingsKey<T> &key) const {
        if (const auto *node = JsonHelper::resolve(m_data, key.path))
            return detail::fromJson<T>(*node, key.defaultValue);
        return key.defaultValue;
    }

    template <typename T>
    void notifyObservers(const char *path, const T &value) {
        std::vector<ObserverEntry> entries;
        {
            std::lock_guard lock(m_mutex);
            auto it = m_observers.find(std::string(path));
            if (it != m_observers.end())
                entries = it->second;
        }
        for (auto &entry : entries)
            entry.callback(&value);
    }

    void loadFromDisk();
    bool saveToDisk();

    QString m_filePath;
    nlohmann::json m_data;
    mutable std::mutex m_mutex;

    std::unordered_map<std::string, std::vector<ObserverEntry>> m_observers;
    int m_nextObserverId = 0;

    QTimer *m_saveTimer = nullptr;
    bool m_dirty = false;
};

} // namespace dstools
