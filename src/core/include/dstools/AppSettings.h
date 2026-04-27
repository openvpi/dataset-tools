#pragma once

/// @file AppSettings.h
/// @brief Type-safe, reactive, JSON-backed persistent settings system.
///
/// Design goals:
///   1. **Type safety** -- each setting is a compile-time typed key, no magic strings at call sites
///   2. **Reactive** -- per-key `valueChanged` signal via Qt signal/slot
///   3. **Immediate persistence** -- every `set()` writes JSON to disk immediately
///   4. **Schema-driven** -- all keys, types, and defaults defined in one place per app
///   5. **Zero QSettings dependency** -- pure nlohmann::json + file I/O
///
/// Usage example:
/// @code
///   // 1. Define your settings schema (typically in a header):
///   namespace MyAppKeys {
///       inline const dstools::SettingsKey<QString> LastDir("General/lastDir", "");
///       inline const dstools::SettingsKey<int>     LastTab("General/lastTab", 0);
///       inline const dstools::SettingsKey<double>  Threshold("Processing/threshold", 0.2);
///       inline const dstools::SettingsKey<bool>    SnapToKey("F0Widget/snapToKey", false);
///       inline const dstools::SettingsKey<QString> ShortcutOpen("Shortcuts/open", "Ctrl+O");
///   }
///
///   // 2. Create the settings store (one per app):
///   dstools::AppSettings settings("MyApp");      // loads <appDir>/config/MyApp.json
///
///   // 3. Read:
///   QString dir = settings.get(MyAppKeys::LastDir);
///   int tab     = settings.get(MyAppKeys::LastTab);
///
///   // 4. Write (automatically persists to disk):
///   settings.set(MyAppKeys::LastDir, QString("/some/path"));
///
///   // 5. React to changes (auto-disconnects when widget is destroyed):
///   settings.observe(MyAppKeys::Threshold, [](double val) {
///       qDebug() << "Threshold changed to" << val;
///   }, myWidget);
/// @endcode

#include <QCoreApplication>
#include <QDir>
#include <QKeySequence>
#include <QObject>
#include <QString>
#include <QVariant>

#include <dstools/JsonHelper.h>
#include <nlohmann/json.hpp>

#include <functional>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace dstools {

// ─── SettingsKey ──────────────────────────────────────────────────────────

/// A typed, named settings key with a default value.
/// The `path` uses '/' as a logical separator (e.g. "General/lastDir").
/// In the JSON file this maps to nested objects: { "General": { "lastDir": "..." } }
///
/// Keys are typically declared as `inline const` at namespace scope in a schema header.
template <typename T>
struct SettingsKey {
    const char *path;
    T defaultValue;

    SettingsKey(const char *path, T defaultValue)
        : path(path), defaultValue(std::move(defaultValue)) {}
};

// ─── JSON conversion helpers ──────────────────────────────────────────────

namespace detail {

    // --- to JSON ---
    inline nlohmann::json toJson(const QString &v) { return v.toStdString(); }
    inline nlohmann::json toJson(const std::string &v) { return v; }
    inline nlohmann::json toJson(int v) { return v; }
    inline nlohmann::json toJson(double v) { return v; }
    inline nlohmann::json toJson(float v) { return static_cast<double>(v); }
    inline nlohmann::json toJson(bool v) { return v; }
    inline nlohmann::json toJson(const QKeySequence &v) { return v.toString().toStdString(); }

    // --- from JSON ---
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

    /// Set a value at a "/"-separated path, creating intermediate objects as needed.
    /// Does nothing for empty paths.
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

// ─── AppSettings ──────────────────────────────────────────────────────────

/// Persistent, reactive settings store backed by a single JSON file.
///
/// Thread safety: all public methods acquire m_mutex. The file I/O is
/// guarded by the same mutex, so concurrent writes are serialized.
/// Signal emission happens outside the lock.
class AppSettings : public QObject {
    Q_OBJECT

public:
    /// @param appName  Used to derive the file path: <appDir>/config/<appName>.json
    /// @param parent   QObject parent (optional)
    explicit AppSettings(const QString &appName, QObject *parent = nullptr);
    ~AppSettings() override = default;

    // ── Typed access ──

    /// Read a typed value. Returns the key's default if not present in JSON.
    template <typename T>
    T get(const SettingsKey<T> &key) const {
        std::lock_guard lock(m_mutex);
        if (const auto *node = JsonHelper::resolve(m_data, key.path))
            return detail::fromJson<T>(*node, key.defaultValue);
        return key.defaultValue;
    }

    /// Write a typed value. Persists to disk immediately. Emits change signal if value differs.
    template <typename T>
    void set(const SettingsKey<T> &key, const T &value) {
        bool changed = false;
        {
            std::lock_guard lock(m_mutex);
            T current = get_unlocked(key);
            if (current != value) {
                detail::assign(m_data, key.path, detail::toJson(value));
                saveToDisk();
                changed = true;
            }
        }
        if (changed) {
            emit keyChanged(QString::fromUtf8(key.path));
            notifyObservers(key.path, value);
        }
    }

    // ── Observation ──

    /// Register a callback invoked when a specific key changes.
    /// The callback receives the new value with the correct type.
    /// Returns a connection ID that can be used with removeObserver().
    ///
    /// If @p context is non-null, the observer is automatically removed when
    /// the context QObject is destroyed (prevents dangling callbacks).
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

    /// Remove an observer by its connection ID.
    void removeObserver(int id);

    // ── Convenience: QKeySequence shortcuts ──

    QKeySequence shortcut(const SettingsKey<QString> &key) const {
        return QKeySequence(get(key));
    }

    void setShortcut(const SettingsKey<QString> &key, const QKeySequence &seq) {
        set(key, seq.toString());
    }

    // ── Bulk / migration ──

    /// Check if a key has been explicitly stored (not just default).
    template <typename T>
    bool contains(const SettingsKey<T> &key) const {
        std::lock_guard lock(m_mutex);
        return JsonHelper::resolve(m_data, key.path) != nullptr;
    }

    /// Remove a key from storage. Next get() will return the key's default.
    /// Emits keyChanged if the key was present.
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

    /// Force re-read from disk (e.g., if external tool edited the file).
    void reload();

    /// Force write to disk (normally not needed since set() writes immediately).
    void flush();

    /// Returns the file path used for persistence.
    QString filePath() const { return m_filePath; }

signals:
    /// Emitted when any key changes. The argument is the key path string.
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
                entries = it->second;  // copy under lock
        }
        for (auto &entry : entries)
            entry.callback(&value);
    }

    void loadFromDisk();
    void saveToDisk();

    QString m_filePath;
    nlohmann::json m_data;
    mutable std::mutex m_mutex;

    std::unordered_map<std::string, std::vector<ObserverEntry>> m_observers;
    int m_nextObserverId = 0;
};

} // namespace dstools
