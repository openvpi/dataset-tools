#pragma once
#include <QUndoStack>
#include <QVBoxLayout>
#include <dsfw/IPageActions.h>
#include <dsfw/IPageLifecycle.h>
#include <dsfw/widgets/TaskWindow.h>

namespace dsfw {

    class PipelineRunner;

}

namespace dstools::labeler {

    class TaskWindowAdapter : public QWidget, public dsfw::IPageActions, public dsfw::IPageLifecycle {
        Q_OBJECT
        Q_INTERFACES(dsfw::IPageActions dsfw::IPageLifecycle)
    public:
        explicit TaskWindowAdapter(dsfw::widgets::TaskWindow *page, QWidget *parent = nullptr);
        ~TaskWindowAdapter() override;

        dsfw::widgets::TaskWindow *innerPage() const;

        bool hasUnsavedChanges() const override;
        void setWorkingDirectory(const QString &dir) override;
        QString workingDirectory() const override;

        void onActivated() override;
        bool onDeactivating() override;
        void onDeactivated() override;
        void onWorkingDirectoryChanged(const QString &newDir) override;
        void onShutdown() override;

        void connectPipelineRunner(dsfw::PipelineRunner *runner);

    private:
        void setupSliceContextMenu();

        dsfw::widgets::TaskWindow *m_page;
        QString m_workingDir;
        QUndoStack m_undoStack;
    };

}
