#pragma once

/// @file AppSettings.h
/// @brief Type-safe, reactive, JSON-backed persistent settings system.
///
/// nlohmann::json is isolated behind the PIMPL boundary (AppSettingsData).
/// No nlohmann headers are included from this public header.

#include <QCoreApplication>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QVariant>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace dstools {

    template <typename T>
    struct SettingsKey {
        const char *path;
        T defaultValue;

        SettingsKey(const char *path, T defaultValue) : path(path), defaultValue(std::move(defaultValue)) {
        }
    };

    class AppSettingsData;

    class AppSettings : public QObject {
        Q_OBJECT

    public:
        explicit AppSettings(const QString &appName, QObject *parent = nullptr);
        ~AppSettings() override;

        // ---- Typed accessor (explicit specializations below) ------------------
        template <typename T>
        T get(const SettingsKey<T> &key) const;

        template <typename T>
        void set(const SettingsKey<T> &key, const T &value);

        // ---- Observer ----------------------------------------------------------
        template <typename T>
        int observe(const SettingsKey<T> &key, std::function<void(const T &)> callback, QObject *context = nullptr) {
            std::lock_guard lock(m_mutex);
            int id = m_nextObserverId++;
            m_observers[std::string(key.path)].push_back(
                {id, [cb = std::move(callback)](const void *ptr) { cb(*static_cast<const T *>(ptr)); }});
            if (context) {
                connect(context, &QObject::destroyed, this, [this, id]() { removeObserver(id); });
            }
            return id;
        }

        void removeObserver(int id);

        // ---- Convenience shortcuts ---------------------------------------------
        QString shortcut(const SettingsKey<QString> &key) const;
        void setShortcut(const SettingsKey<QString> &key, const QString &seq);

        // ---- Key presence -------------------------------------------------------
        template <typename T>
        bool contains(const SettingsKey<T> &key) const {
            std::lock_guard lock(m_mutex);
            return hasKey(key.path);
        }

        template <typename T>
        void remove(const SettingsKey<T> &key) {
            bool removed = false;
            {
                std::lock_guard lock(m_mutex);
                removed = eraseKey(key.path);
                if (removed)
                    doFlush();
            }
            if (removed) {
                emit keyChanged(QString::fromUtf8(key.path));
                notifyObservers(key.path, key.defaultValue);
            }
        }

        // ---- Lifecycle ----------------------------------------------------------
        void reload();
        void flush();
        QString filePath() const;

        // ---- Raw JSON string access (for complex nested data) -----------------
        QString getRawString(const char *path, const QString &fallback) const;
        void setRawString(const char *path, const QString &value);

    signals:
        void keyChanged(const QString &keyPath);

    private:
        struct ObserverEntry {
            int id;
            std::function<void(const void *)> callback;
        };

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
        void doFlush();
        void migrateIfNeeded();

        bool hasKey(const char *path) const;
        bool eraseKey(const char *path);

        QString getValue(const char *path, const QString &defaultValue) const;
        int getValue(const char *path, int defaultValue) const;
        double getValue(const char *path, double defaultValue) const;
        float getValue(const char *path, float defaultValue) const;
        bool getValue(const char *path, bool defaultValue) const;
        std::string getValue(const char *path, const std::string &defaultValue) const;

        void setValue(const char *path, const QString &value);
        void setValue(const char *path, int value);
        void setValue(const char *path, double value);
        void setValue(const char *path, float value);
        void setValue(const char *path, bool value);
        void setValue(const char *path, const std::string &value);

        std::unique_ptr<AppSettingsData> m_impl;
        mutable std::mutex m_mutex;

        std::unordered_map<std::string, std::vector<ObserverEntry>> m_observers;
        int m_nextObserverId = 0;

        QTimer *m_saveTimer = nullptr;
    };

    // ---- get<T>() explicit specializations ------------------------------------
    template <>
    inline QString AppSettings::get<QString>(const SettingsKey<QString> &key) const {
        std::lock_guard lock(m_mutex);
        return getValue(key.path, key.defaultValue);
    }

    template <>
    inline int AppSettings::get<int>(const SettingsKey<int> &key) const {
        std::lock_guard lock(m_mutex);
        return getValue(key.path, key.defaultValue);
    }

    template <>
    inline double AppSettings::get<double>(const SettingsKey<double> &key) const {
        std::lock_guard lock(m_mutex);
        return getValue(key.path, key.defaultValue);
    }

    template <>
    inline float AppSettings::get<float>(const SettingsKey<float> &key) const {
        std::lock_guard lock(m_mutex);
        return getValue(key.path, key.defaultValue);
    }

    template <>
    inline bool AppSettings::get<bool>(const SettingsKey<bool> &key) const {
        std::lock_guard lock(m_mutex);
        return getValue(key.path, key.defaultValue);
    }

    template <>
    inline std::string AppSettings::get<std::string>(const SettingsKey<std::string> &key) const {
        std::lock_guard lock(m_mutex);
        return getValue(key.path, key.defaultValue);
    }

    // ---- set<T>() explicit specializations ------------------------------------
    template <>
    inline void AppSettings::set<QString>(const SettingsKey<QString> &key, const QString &value) {
        bool changed = false;
        {
            std::lock_guard lock(m_mutex);
            if (getValue(key.path, key.defaultValue) != value) {
                setValue(key.path, value);
                QMetaObject::invokeMethod(m_saveTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
                changed = true;
            }
        }
        if (changed) {
            emit keyChanged(QString::fromUtf8(key.path));
            notifyObservers(key.path, value);
        }
    }

    template <>
    inline void AppSettings::set<int>(const SettingsKey<int> &key, const int &value) {
        bool changed = false;
        {
            std::lock_guard lock(m_mutex);
            if (getValue(key.path, key.defaultValue) != value) {
                setValue(key.path, value);
                QMetaObject::invokeMethod(m_saveTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
                changed = true;
            }
        }
        if (changed) {
            emit keyChanged(QString::fromUtf8(key.path));
            notifyObservers(key.path, value);
        }
    }

    template <>
    inline void AppSettings::set<double>(const SettingsKey<double> &key, const double &value) {
        bool changed = false;
        {
            std::lock_guard lock(m_mutex);
            if (getValue(key.path, key.defaultValue) != value) {
                setValue(key.path, value);
                QMetaObject::invokeMethod(m_saveTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
                changed = true;
            }
        }
        if (changed) {
            emit keyChanged(QString::fromUtf8(key.path));
            notifyObservers(key.path, value);
        }
    }

    template <>
    inline void AppSettings::set<float>(const SettingsKey<float> &key, const float &value) {
        bool changed = false;
        {
            std::lock_guard lock(m_mutex);
            if (getValue(key.path, key.defaultValue) != value) {
                setValue(key.path, value);
                QMetaObject::invokeMethod(m_saveTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
                changed = true;
            }
        }
        if (changed) {
            emit keyChanged(QString::fromUtf8(key.path));
            notifyObservers(key.path, value);
        }
    }

    template <>
    inline void AppSettings::set<bool>(const SettingsKey<bool> &key, const bool &value) {
        bool changed = false;
        {
            std::lock_guard lock(m_mutex);
            if (getValue(key.path, key.defaultValue) != value) {
                setValue(key.path, value);
                QMetaObject::invokeMethod(m_saveTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
                changed = true;
            }
        }
        if (changed) {
            emit keyChanged(QString::fromUtf8(key.path));
            notifyObservers(key.path, value);
        }
    }

    template <>
    inline void AppSettings::set<std::string>(const SettingsKey<std::string> &key, const std::string &value) {
        bool changed = false;
        {
            std::lock_guard lock(m_mutex);
            if (getValue(key.path, key.defaultValue) != value) {
                setValue(key.path, value);
                QMetaObject::invokeMethod(m_saveTimer, qOverload<>(&QTimer::start), Qt::QueuedConnection);
                changed = true;
            }
        }
        if (changed) {
            emit keyChanged(QString::fromUtf8(key.path));
            notifyObservers(key.path, value);
        }
    }

    inline QString AppSettings::shortcut(const SettingsKey<QString> &key) const {
        return get(key);
    }

    inline void AppSettings::setShortcut(const SettingsKey<QString> &key, const QString &seq) {
        set(key, seq);
    }

} // namespace dstools