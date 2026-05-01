#pragma once

/// @file RecentFiles.h
/// @brief Persistent most-recently-used file list backed by QSettings.

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>

namespace dstools {

/// @brief Manages a persistent list of recently opened files.
class RecentFiles : public QObject {
    Q_OBJECT
public:
    /// @param settingsKey QSettings key for persistence.
    /// @param maxCount Maximum number of entries.
    explicit RecentFiles(const QString &settingsKey = "RecentFiles",
                         int maxCount = 10, QObject *parent = nullptr);

    /// @brief Add a file path to the top of the list.
    void addFile(const QString &filePath);
    /// @brief Remove a file path from the list.
    void removeFile(const QString &filePath);
    /// @brief Clear the entire list.
    void clear();
    /// @brief Return the current file list (most recent first).
    QStringList files() const;
    /// @brief Return the maximum number of entries.
    int maxCount() const;
    /// @brief Set the maximum number of entries.
    void setMaxCount(int count);

signals:
    /// @brief Emitted when the list changes.
    void listChanged();

private:
    void load();
    void save();
    QString m_settingsKey;
    int m_maxCount;
    QStringList m_files;
};

} // namespace dstools
