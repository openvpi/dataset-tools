#pragma once
#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dstools/TaskWindow.h>
#include <QVBoxLayout>

namespace dstools::labeler {

class TaskWindowAdapter : public QWidget, public IPageActions, public IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)
public:
    explicit TaskWindowAdapter(dstools::widgets::TaskWindow *page, QWidget *parent = nullptr);
    ~TaskWindowAdapter() override;

    dstools::widgets::TaskWindow *innerPage() const;

    QList<QAction *> editActions() const override;
    QList<QAction *> viewActions() const override;
    QList<QAction *> toolActions() const override;
    bool hasUnsavedChanges() const override;
    bool save() override;
    void setWorkingDirectory(const QString &dir) override;
    QString workingDirectory() const override;

    void onActivated() override;
    bool onDeactivating() override;
    void onDeactivated() override;
    void onWorkingDirectoryChanged(const QString &newDir) override;
    void onShutdown() override;

private:
    dstools::widgets::TaskWindow *m_page;
    QString m_workingDir;
};

}
