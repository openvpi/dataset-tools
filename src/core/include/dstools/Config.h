#pragma once
#include <QSettings>
#include <QKeySequence>
#include <QString>

namespace dstools {

/// Unified configuration wrapper. Based on QSettings INI format.
/// Configuration file located at <app_dir>/config/<appName>.ini
class Config {
public:
    /// @param appName Config file name (without extension), e.g. "DatasetTools"
    explicit Config(const QString &appName);

    // Basic read/write
    QString getString(const QString &key, const QString &defaultValue = {}) const;
    void setString(const QString &key, const QString &value);
    int getInt(const QString &key, int defaultValue = 0) const;
    void setInt(const QString &key, int value);
    double getDouble(const QString &key, double defaultValue = 0.0) const;
    void setDouble(const QString &key, double value);
    bool getBool(const QString &key, bool defaultValue = false) const;
    void setBool(const QString &key, bool value);

    // Shortcuts
    QKeySequence shortcut(const QString &action, const QKeySequence &defaultSeq) const;
    void setShortcut(const QString &action, const QKeySequence &seq);

    // Group operations
    void beginGroup(const QString &group);
    void endGroup();

    // Sync to disk
    void sync();

    // Low-level access (backward compatibility)
    QSettings &settings();

private:
    QSettings m_settings;
};

} // namespace dstools
