#pragma once

/// @file IPageActions.h
/// @brief Page actions interface for menu bar, status bar, drag-drop, and save integration.

#include <QAction>
#include <QList>
#include <QMenuBar>
#include <QString>

class QDragEnterEvent;
class QDropEvent;
class QWidget;

namespace dstools::labeler {

/// @brief Interface for pages that contribute menus, status bar content, and drag-drop handling.
///
/// Implement this interface on page widgets so that AppShell can query
/// for menu bar content, status bar widgets, and drag-drop support.
class IPageActions {
public:
    virtual ~IPageActions() = default;

    /// @brief Check whether this page has unsaved changes.
    /// @return True if there are unsaved modifications.
    virtual bool hasUnsavedChanges() const { return false; }
    /// @brief Save the page's current state.
    /// @return True on success.
    virtual bool save() { return true; }

    /// @brief Set the working directory for this page.
    /// @param dir Absolute directory path (unused in default implementation).
    virtual void setWorkingDirectory(const QString &) {}
    /// @brief Return the page's working directory.
    /// @return Working directory path.
    virtual QString workingDirectory() const { return {}; }

    /// @brief Create a menu bar for this page.
    /// @param parent Parent widget for the menu bar.
    /// @return New QMenuBar, or nullptr if the page has no menus.
    virtual QMenuBar *createMenuBar(QWidget *parent) { Q_UNUSED(parent); return nullptr; }
    /// @brief Create status bar content for this page.
    /// @param parent Parent widget.
    /// @return Widget to embed in the status bar, or nullptr.
    virtual QWidget *createStatusBarContent(QWidget *parent) { Q_UNUSED(parent); return nullptr; }
    /// @brief Return a custom window title for this page.
    /// @return Title string, or empty for default.
    virtual QString windowTitle() const { return {}; }
    /// @brief Check whether this page supports drag-and-drop.
    /// @return True if drag-drop is supported.
    virtual bool supportsDragDrop() const { return false; }
    /// @brief Handle a drag-enter event forwarded from the shell.
    /// @param event The drag-enter event.
    virtual void handleDragEnter(QDragEnterEvent *event) { Q_UNUSED(event); }
    /// @brief Handle a drop event forwarded from the shell.
    /// @param event The drop event.
    virtual void handleDrop(QDropEvent *event) { Q_UNUSED(event); }
};

} // namespace dstools::labeler

#define LABELER_PAGE_ACTIONS_IID "dstools.labeler.IPageActions"
Q_DECLARE_INTERFACE(dstools::labeler::IPageActions, LABELER_PAGE_ACTIONS_IID)
