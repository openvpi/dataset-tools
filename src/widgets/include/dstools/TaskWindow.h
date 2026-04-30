#pragma once
#include <dstools/WidgetsGlobal.h>
#include <QWidget>
#include <QStringList>
#include <QListWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QThreadPool>

#include <atomic>

namespace dstools::widgets {

/// Batch task window base class.
/// Replaces original AsyncTask::AsyncTaskWindow (changed from QMainWindow to QWidget for Tab embedding).
class DSTOOLS_WIDGETS_API TaskWindow : public QWidget {
    Q_OBJECT
public:
    enum LayoutMode { Classic, PipelineStyle };

    explicit TaskWindow(QWidget *parent = nullptr);
    explicit TaskWindow(LayoutMode mode, QWidget *parent = nullptr);
    ~TaskWindow() override;

    void setMaxThreadCount(int count);
    QListWidget *taskList() const;
    void setRunButtonText(const QString &text);
    QThreadPool *threadPool() const;

public slots:
    void addFiles();
    void addFolder();
    void removeSelected();
    void clearList();
    void slot_oneFinished(const QString &filename, const QString &info);
    void slot_oneFailed(const QString &filename, const QString &error);

signals:
    void allTasksFinished();

protected:
    virtual void init() {}
    virtual void runTask() = 0;
    virtual void onTaskFinished() {}

    void setLayoutMode(LayoutMode mode);
    void setProgressBarVisible(bool visible);
    void addTopWidget(QWidget *widget);

    // UI components (accessible by subclasses)
    QVBoxLayout *m_rightPanel;
    QHBoxLayout *m_topLayout;
    QListWidget *m_taskListWidget;
    QProgressBar *m_progressBar;
    QPlainTextEdit *m_logOutput;
    QPushButton *m_runBtn;

    // State
    int m_totalTasks = 0;
    std::atomic<int> m_finishedTasks{0};
    std::atomic<int> m_errorTasks{0};
    QStringList m_errorDetails;
    std::atomic<bool> m_isRunning{false};

    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    QThreadPool *m_threadPool;
    LayoutMode m_layoutMode = Classic;
    void setupUI();
    void setupClassicUI();
    void setupPipelineStyleUI();
    void updateProgress();
    void onRunClicked();
};

} // namespace dstools::widgets
