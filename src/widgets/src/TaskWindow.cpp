#include <dstools/TaskWindow.h>

#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QThreadPool>
#include <QVBoxLayout>
#include <QDir>

namespace dstools::widgets {

TaskWindow::TaskWindow(QWidget *parent)
    : QWidget(parent), m_threadPool(new QThreadPool(this)) {
    m_threadPool->setMaxThreadCount(1);
    setupUI();
    setAcceptDrops(true);
}

TaskWindow::TaskWindow(LayoutMode mode, QWidget *parent)
    : QWidget(parent), m_threadPool(new QThreadPool(this)), m_layoutMode(mode) {
    m_threadPool->setMaxThreadCount(1);
    setupUI();
    setAcceptDrops(true);
}

TaskWindow::~TaskWindow() = default;

void TaskWindow::setLayoutMode(LayoutMode mode) {
    m_layoutMode = mode;
}

void TaskWindow::setProgressBarVisible(bool visible) {
    m_progressBar->setVisible(visible);
}

void TaskWindow::setupUI() {
    if (m_layoutMode == PipelineStyle)
        setupPipelineStyleUI();
    else
        setupClassicUI();
}

void TaskWindow::setupClassicUI() {
    auto *mainLayout = new QHBoxLayout(this);

    // Left panel: task list + buttons
    auto *leftPanel = new QVBoxLayout();

    m_taskListWidget = new QListWidget();
    leftPanel->addWidget(m_taskListWidget);

    auto *listBtns = new QHBoxLayout();
    auto *addFileBtn = new QPushButton(tr("Add Files"));
    auto *addFolderBtn = new QPushButton(tr("Add Folder"));
    auto *removeBtn = new QPushButton(tr("Remove"));
    auto *clearBtn = new QPushButton(tr("Clear"));
    listBtns->addWidget(addFileBtn);
    listBtns->addWidget(addFolderBtn);
    listBtns->addWidget(removeBtn);
    listBtns->addWidget(clearBtn);
    leftPanel->addLayout(listBtns);

    connect(addFileBtn, &QPushButton::clicked, this, &TaskWindow::addFiles);
    connect(addFolderBtn, &QPushButton::clicked, this, &TaskWindow::addFolder);
    connect(removeBtn, &QPushButton::clicked, this, &TaskWindow::removeSelected);
    connect(clearBtn, &QPushButton::clicked, this, &TaskWindow::clearList);

    mainLayout->addLayout(leftPanel, 1);

    // Right panel: custom widgets + progress + log + run button
    auto *rightContainer = new QVBoxLayout();

    m_topLayout = new QHBoxLayout();
    rightContainer->addLayout(m_topLayout);

    m_rightPanel = new QVBoxLayout();
    rightContainer->addLayout(m_rightPanel);

    m_progressBar = new QProgressBar();
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    rightContainer->addWidget(m_progressBar);

    m_logOutput = new QPlainTextEdit();
    m_logOutput->setReadOnly(true);
    rightContainer->addWidget(m_logOutput);

    m_runBtn = new QPushButton(tr("Run"));
    connect(m_runBtn, &QPushButton::clicked, this, &TaskWindow::onRunClicked);
    rightContainer->addWidget(m_runBtn);

    mainLayout->addLayout(rightContainer, 2);
}

void TaskWindow::setupPipelineStyleUI() {
    auto *mainLayout = new QVBoxLayout(this);

    // Top area: file list (left) + parameters (right) side-by-side
    auto *topRow = new QHBoxLayout();

    // Left panel: task list + buttons
    auto *leftPanel = new QVBoxLayout();

    m_taskListWidget = new QListWidget();
    leftPanel->addWidget(m_taskListWidget);

    auto *listBtns = new QHBoxLayout();
    auto *addFileBtn = new QPushButton(tr("Add Files"));
    auto *addFolderBtn = new QPushButton(tr("Add Folder"));
    auto *removeBtn = new QPushButton(tr("Remove"));
    auto *clearBtn = new QPushButton(tr("Clear"));
    listBtns->addWidget(addFileBtn);
    listBtns->addWidget(addFolderBtn);
    listBtns->addWidget(removeBtn);
    listBtns->addWidget(clearBtn);
    leftPanel->addLayout(listBtns);

    connect(addFileBtn, &QPushButton::clicked, this, &TaskWindow::addFiles);
    connect(addFolderBtn, &QPushButton::clicked, this, &TaskWindow::addFolder);
    connect(removeBtn, &QPushButton::clicked, this, &TaskWindow::removeSelected);
    connect(clearBtn, &QPushButton::clicked, this, &TaskWindow::clearList);

    topRow->addLayout(leftPanel, 1);

    // Right panel: custom widgets + run button
    auto *rightContainer = new QVBoxLayout();

    m_topLayout = new QHBoxLayout();
    rightContainer->addLayout(m_topLayout);

    m_rightPanel = new QVBoxLayout();
    rightContainer->addLayout(m_rightPanel);

    m_runBtn = new QPushButton(tr("Run"));
    connect(m_runBtn, &QPushButton::clicked, this, &TaskWindow::onRunClicked);
    rightContainer->addWidget(m_runBtn);

    topRow->addLayout(rightContainer, 2);
    mainLayout->addLayout(topRow, 1);

    // Bottom area: progress bar + log output (full width)
    auto *bottomPanel = new QVBoxLayout();

    m_progressBar = new QProgressBar();
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(100);
    m_progressBar->setValue(0);
    bottomPanel->addWidget(m_progressBar);

    m_logOutput = new QPlainTextEdit();
    m_logOutput->setReadOnly(true);
    bottomPanel->addWidget(m_logOutput);

    mainLayout->addLayout(bottomPanel, 1);
}

void TaskWindow::setMaxThreadCount(int count) {
    m_threadPool->setMaxThreadCount(count);
}

QListWidget *TaskWindow::taskList() const {
    return m_taskListWidget;
}

void TaskWindow::setRunButtonText(const QString &text) {
    m_runBtn->setText(text);
}

QThreadPool *TaskWindow::threadPool() const {
    return m_threadPool;
}

void TaskWindow::addFiles() {
    if (m_isRunning) return;
    QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Add Files"), QString(),
        tr("Audio Files (*.wav *.mp3 *.m4a *.flac);;All Files (*.*)"));
    for (const QString &file : files) {
        auto *item = new QListWidgetItem(QFileInfo(file).fileName());
        item->setData(Qt::UserRole + 1, file);
        m_taskListWidget->addItem(item);
    }
}

void TaskWindow::addFolder() {
    if (m_isRunning) return;
    QString dir = QFileDialog::getExistingDirectory(this, tr("Add Folder"));
    if (dir.isEmpty()) return;
    QDir d(dir);
    QStringList files = d.entryList({"*.wav", "*.mp3", "*.m4a", "*.flac"}, QDir::Files);
    for (const QString &file : files) {
        auto *item = new QListWidgetItem(file);
        item->setData(Qt::UserRole + 1, d.absoluteFilePath(file));
        m_taskListWidget->addItem(item);
    }
}

void TaskWindow::removeSelected() {
    if (m_isRunning) return;
    qDeleteAll(m_taskListWidget->selectedItems());
}

void TaskWindow::clearList() {
    if (m_isRunning) return;
    m_taskListWidget->clear();
}

void TaskWindow::slot_oneFinished(const QString &filename, const QString &info) {
    m_finishedTasks++;
    m_logOutput->appendPlainText(filename + ": " + info);
    updateProgress();
    if (m_finishedTasks == m_totalTasks) {
        m_isRunning = false;
        m_runBtn->setEnabled(true);
        onTaskFinished();
        emit allTasksFinished();
    }
}

void TaskWindow::slot_oneFailed(const QString &filename, const QString &error) {
    m_finishedTasks++;
    m_errorTasks++;
    m_errorDetails.append(filename + ": " + error);
    m_logOutput->appendPlainText("[ERROR] " + filename + ": " + error);
    updateProgress();
    if (m_finishedTasks == m_totalTasks) {
        m_isRunning = false;
        m_runBtn->setEnabled(true);
        onTaskFinished();
        emit allTasksFinished();
    }
}

void TaskWindow::addTopWidget(QWidget *widget) {
    m_topLayout->addWidget(widget);
}

void TaskWindow::updateProgress() {
    if (m_totalTasks > 0) {
        m_progressBar->setValue(m_finishedTasks * 100 / m_totalTasks);
    }
}

void TaskWindow::onRunClicked() {
    if (m_isRunning) return;
    m_isRunning = true;
    m_runBtn->setEnabled(false);
    runTask();
}

void TaskWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void TaskWindow::dropEvent(QDropEvent *event) {
    if (m_isRunning) return;
    const QMimeData *mime = event->mimeData();
    if (!mime->hasUrls()) return;
    for (const QUrl &url : mime->urls()) {
        if (!url.isLocalFile()) continue;
        QString path = url.toLocalFile();
        QFileInfo info(path);
        if (info.isFile()) {
            auto *item = new QListWidgetItem(info.fileName());
            item->setData(Qt::UserRole + 1, path);
            m_taskListWidget->addItem(item);
        } else if (info.isDir()) {
            QDir d(path);
            for (const QString &file : d.entryList({"*.wav", "*.mp3", "*.m4a", "*.flac"}, QDir::Files)) {
                auto *item = new QListWidgetItem(file);
                item->setData(Qt::UserRole + 1, d.absoluteFilePath(file));
                m_taskListWidget->addItem(item);
            }
        }
    }
    event->acceptProposedAction();
}

} // namespace dstools::widgets
