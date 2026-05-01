#pragma once
/// @file TaskWindow.h
/// @brief Base window for batch file processing tasks with progress tracking.

#include <dsfw/widgets/WidgetsGlobal.h>
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

namespace dsfw::widgets {

/// @brief Base widget for batch file processing with drag-and-drop, progress, and logging.
class DSFW_WIDGETS_API TaskWindow : public QWidget {
    Q_OBJECT
public:
    /// @brief Layout mode for the task window.
    enum LayoutMode { Classic, PipelineStyle };

    /// @brief Construct a TaskWindow with Classic layout.
    /// @param parent Parent widget.
    explicit TaskWindow(QWidget *parent = nullptr);

    /// @brief Construct a TaskWindow with a specific layout mode.
    /// @param mode Layout mode.
    /// @param parent Parent widget.
    explicit TaskWindow(LayoutMode mode, QWidget *parent = nullptr);
    ~TaskWindow() override;

    /// @brief Set the maximum number of concurrent threads.
    /// @param count Maximum thread count.
    void setMaxThreadCount(int count);

    /// @brief Get the file list widget.
    /// @return Pointer to the task list widget.
    QListWidget *taskList() const;

    /// @brief Set the text on the run button.
    /// @param text Button label.
    void setRunButtonText(const QString &text);

    /// @brief Get the thread pool used for task execution.
    /// @return Pointer to the thread pool.
    QThreadPool *threadPool() const;

public slots:
    /// @brief Open a file dialog and add selected files to the list.
    void addFiles();

    /// @brief Open a folder dialog and add all files from it.
    void addFolder();

    /// @brief Remove selected items from the file list.
    void removeSelected();

    /// @brief Clear the entire file list.
    void clearList();

    /// @brief Handle completion of a single file task.
    /// @param filename Name of the completed file.
    /// @param info Completion info message.
    void slot_oneFinished(const QString &filename, const QString &info);

    /// @brief Handle failure of a single file task.
    /// @param filename Name of the failed file.
    /// @param error Error message.
    void slot_oneFailed(const QString &filename, const QString &error);

signals:
    /// @brief Emitted when all queued tasks have finished.
    void allTasksFinished();

protected:
    virtual void init() {}
    virtual void runTask() = 0;
    virtual void onTaskFinished() {}

    void setLayoutMode(LayoutMode mode);
    void setProgressBarVisible(bool visible);
    void addTopWidget(QWidget *widget);

    QVBoxLayout *m_rightPanel;
    QHBoxLayout *m_topLayout;
    QListWidget *m_taskListWidget;
    QProgressBar *m_progressBar;
    QPlainTextEdit *m_logOutput;
    QPushButton *m_runBtn;

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

} // namespace dsfw::widgets
