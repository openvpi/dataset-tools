#ifndef ASYNCTASKWINDOW_H
#define ASYNCTASKWINDOW_H

#include <QBoxLayout>
#include <QListWidget>
#include <QMainWindow>
#include <QMenu>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QThreadPool>

#include <AsyncTaskWindowGlobal.h>

namespace AsyncTask {
    class ASYNC_TASK_WINDOW_EXPORT AsyncTaskWindow : public QMainWindow {
        Q_OBJECT
    public:
        explicit AsyncTaskWindow(QWidget *parent = nullptr);
        ~AsyncTaskWindow() override;

        void setMaxThreadCount(int count) const;

        QListWidget *taskList() const {
            return m_taskList;
        }

        void setRunButtonText(const QString &text) const;

    public slots:
        void slot_addFile();
        void slot_addFolder();
        void slot_removeSelected() const;
        void slot_clearList() const;
        void slot_aboutApp();
        void slot_aboutQt();

        void slot_oneFinished(const QString &filename, const QString &msg = QString());
        void slot_oneFailed(const QString &filename, const QString &msg);

    protected:
        virtual void init() = 0;
        virtual void runTask() = 0;

        virtual void onTaskFinished();

        void addFiles(const QStringList &paths) const;
        void addFolder(const QString &path) const;

        QVBoxLayout *m_rightPanel;
        QPlainTextEdit *m_logOutput;
        QProgressBar *m_progressBar;
        QThreadPool *m_threadPool;

        int m_totalTasks = 0;
        int m_finishedTasks = 0;
        int m_errorTasks = 0;
        QStringList m_errorDetails;


    private:
        void setupCommonUI();
        void dragEnterEvent(QDragEnterEvent *event) override;
        void dropEvent(QDropEvent *event) override;
        void closeEvent(QCloseEvent *event) override;
        void createMenus();

        QMenu *m_fileMenu;
        QAction *m_addFileAction;
        QAction *m_addFolderAction;
        QMenu *m_helpMenu;
        QAction *m_aboutAppAction;
        QAction *m_aboutQtAction;

        QListWidget *m_taskList;
        QPushButton *m_runBtn;
    };

} // namespace AsyncTask

#endif // ASYNCTASKWINDOW_H