#pragma once
#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dstools/TaskWindow.h>
#include <QVBoxLayout>
#include <QUndoStack>

namespace dstools {

class PipelineRunner;

}

namespace dstools::labeler {

class TaskWindowAdapter : public QWidget, public IPageActions, public IPageLifecycle {
    Q_OBJECT
    Q_INTERFACES(dstools::labeler::IPageActions dstools::labeler::IPageLifecycle)
public:
    explicit TaskWindowAdapter(dstools::widgets::TaskWindow *page, QWidget *parent = nullptr);
    ~TaskWindowAdapter() override;

    dstools::widgets::TaskWindow *innerPage() const;

    bool hasUnsavedChanges() const override;
    void setWorkingDirectory(const QString &dir) override;
    QString workingDirectory() const override;

    void onActivated() override;
    bool onDeactivating() override;
    void onDeactivated() override;
    void onWorkingDirectoryChanged(const QString &newDir) override;
    void onShutdown() override;

    void connectPipelineRunner(dstools::PipelineRunner *runner);

private:
    void setupSliceContextMenu();

    dstools::widgets::TaskWindow *m_page;
    QString m_workingDir;
    QUndoStack m_undoStack;
};

}
