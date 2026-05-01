#pragma once

/// @file RecentFilesManager.h
/// @brief Most-recently-used file list with QMenu integration and AppSettings persistence.

#include <QObject>
#include <QPointer>
#include <QStringList>

class QMenu;
class QWidget;

namespace dstools {
class AppSettings;
}

namespace dsfw {

/// @brief Manages a most-recently-used file list with automatic persistence.
///
/// @code
/// RecentFilesManager recent(&settings);
/// recent.addFile("/path/to/file.ds");
/// auto *menu = recent.createMenu("Recent Files", this);
/// fileMenu->addMenu(menu);
/// connect(&recent, &RecentFilesManager::fileTriggered, this, &MyPage::openFile);
/// @endcode
class RecentFilesManager : public QObject {
    Q_OBJECT
public:
    /// @brief Construct the manager backed by the given settings.
    /// @param settings AppSettings instance for persistence.
    /// @param parent Optional QObject parent.
    explicit RecentFilesManager(dstools::AppSettings *settings, QObject *parent = nullptr);

    /// @brief Add a file to the top of the list. Removes duplicates.
    /// @param filePath Absolute file path.
    void addFile(const QString &filePath);

    /// @brief Return the current file list, most recent first.
    QStringList fileList() const;

    /// @brief Clear all recent files.
    void clear();

    /// @brief Set maximum number of entries (default: 10).
    /// @param max Maximum entry count.
    void setMaxCount(int max);

    /// @brief Return the maximum entry count.
    int maxCount() const;

    /// @brief Create a QMenu populated with recent file entries.
    ///
    /// The menu is automatically updated when files are added/removed.
    /// Triggering an entry emits fileTriggered().
    /// @param title Menu title.
    /// @param parent Parent widget for the menu.
    /// @return Owned QMenu (caller takes ownership).
    QMenu *createMenu(const QString &title, QWidget *parent = nullptr);

signals:
    /// @brief Emitted when a recent file entry is triggered.
    /// @param filePath The file path that was selected.
    void fileTriggered(const QString &filePath);

    /// @brief Emitted when the file list changes.
    void listChanged();

private:
    void load();
    void save();
    void rebuildMenus();

    dstools::AppSettings *m_settings = nullptr;
    QStringList m_files;
    int m_maxCount = 10;
    QList<QPointer<QMenu>> m_menus;
};

} // namespace dsfw
