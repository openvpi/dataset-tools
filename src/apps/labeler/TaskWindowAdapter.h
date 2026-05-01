/// @file TaskWindowAdapter.h
/// @brief Adapter wrapping TaskWindow as a page with lifecycle support.

#pragma once
#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dstools/TaskWindow.h>
#include <QVBoxLayout>

namespace dstools::labeler {

/// @brief Bridges a TaskWindow widget to IPageActions and IPageLifecycle interfaces for use in AppShell.
class TaskWindowAdapter : public QWidget, public IPageActions, public IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)
public:
    /// @brief Constructs the adapter around an existing TaskWindow.
    /// @param page The TaskWindow to wrap.
    /// @param parent Optional parent widget.
    explicit TaskWindowAdapter(dstools::widgets::TaskWindow *page, QWidget *parent = nullptr);
    ~TaskWindowAdapter() override;

    /// @brief Returns the wrapped TaskWindow.
    /// @return Pointer to the inner TaskWindow.
    dstools::widgets::TaskWindow *innerPage() const;

    bool hasUnsavedChanges() const override;
    void setWorkingDirectory(const QString &dir) override;
    QString workingDirectory() const override;

    void onActivated() override;
    bool onDeactivating() override;
    void onDeactivated() override;
    void onWorkingDirectoryChanged(const QString &newDir) override;
    void onShutdown() override;

private:
    dstools::widgets::TaskWindow *m_page;  ///< Wrapped TaskWindow instance.
    QString m_workingDir;                  ///< Current working directory.
};

}
