#pragma once

/// @file AppShell.h
/// @brief Unified application window shell with icon sidebar navigation and page management.

#include "dsfw/widgets/ToastNotification.h"

#include <QList>
#include <QMainWindow>

#include <memory>

class QStackedWidget;
class QStatusBar;
class QAction;
class QMenuBar;
class QShowEvent;

namespace dsfw {

class AppSettings;
class IconNavBar;
class AudioPlaybackManager;

/// @brief Main application window with sidebar navigation and stacked page management.
///
/// Supports single-page and multi-page modes. Pages implementing IPageActions
/// and IPageLifecycle get automatic menu bar, status bar, and lifecycle integration.
/// Drag-and-drop events are forwarded to the active page if it supports them.
///
/// @code
/// AppShell shell;
/// shell.addPage(new EditorPage, "editor", QIcon(":/editor.svg"), "Editor");
/// shell.addPage(new PipelinePage, "pipeline", QIcon(":/pipe.svg"), "Pipeline");
/// shell.show();
/// @endcode
class AppShell : public QMainWindow {
    Q_OBJECT
public:
    /// @brief Construct the application shell.
    /// @param parent Optional parent widget.
    explicit AppShell(QWidget* parent = nullptr);
    ~AppShell() override;

    // Page management

    /// @brief Add a page to the shell.
    /// @param page Widget for the page content.
    /// @param id Unique string identifier for the page.
    /// @param icon Optional icon shown in the sidebar.
    /// @param label Optional tooltip/label for the sidebar item.
    /// @return Index of the newly added page.
    int addPage(QWidget* page, const QString& id, const QIcon& icon = {}, const QString& label = {});
    /// @brief Return the total number of pages.
    /// @return Page count.
    int pageCount() const;
    /// @brief Return the index of the currently visible page.
    /// @return Current page index, or -1 if no pages.
    int currentPageIndex() const;
    /// @brief Return the currently visible page widget.
    /// @return Page widget pointer, or nullptr.
    QWidget* currentPage() const;
    /// @brief Return the page widget at the given index.
    /// @param index Page index.
    /// @return Page widget pointer.
    QWidget* pageAt(int index) const;
    /// @brief Switch to the page at the given index.
    /// @param index Page index.
    void setCurrentPage(int index);

    // Global actions (persist across page switches, prepended to menu bar)

    /// @brief Add global menu actions that appear on all pages.
    /// @param actions List of QAction pointers (caller retains ownership).
    void addGlobalMenuActions(const QList<QAction*>& actions);

    // Working directory

    /// @brief Attach an AppSettings instance for automatic geometry persistence.
    /// @param settings Pointer to an AppSettings instance (caller retains ownership).
    void setSettings(AppSettings* settings);

    /// @brief Set the shared working directory and notify all pages.
    /// @param dir Absolute directory path.
    void setWorkingDirectory(const QString& dir);
    /// @brief Return the current working directory.
    /// @return Working directory path.
    QString workingDirectory() const;

    /// @brief Show a non-modal toast notification.
    void showToast(widgets::ToastType type, const QString& message, int timeoutMs = 3000);

    /// @brief Return the audio playback arbitration manager.
    AudioPlaybackManager* audioPlaybackManager() const;

private slots:
    void onChildPlayRequested();
    void onChildPlayStopped();

signals:
    /// @brief Emitted when the active page changes.
    /// @param index New page index.
    void currentPageChanged(int index);
    /// @brief Emitted when the working directory changes.
    /// @param dir New working directory path.
    void workingDirectoryChanged(const QString& dir);

protected:
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void onPageSwitched(int index);
    void rebuildMenuBar();
    void rebuildStatusBar();
    bool hasAnyUnsavedChanges() const;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace dsfw
