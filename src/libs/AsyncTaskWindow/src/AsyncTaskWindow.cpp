#include "AsyncTaskWindow.h"
#include <QApplication>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPushButton>
#include <QStandardPaths>

namespace AsyncTask {

    AsyncTaskWindow::AsyncTaskWindow(QWidget *parent) : QMainWindow(parent), m_threadPool(new QThreadPool(this)) {
        m_threadPool->setMaxThreadCount(1);
        setAcceptDrops(true);

        createMenus();
        setupCommonUI();

        resize(960, 720);
    }

    AsyncTaskWindow::~AsyncTaskWindow() = default;

    void AsyncTaskWindow::setMaxThreadCount(const int count) const {
        m_threadPool->setMaxThreadCount(count);
    }

    void AsyncTaskWindow::setRunButtonText(const QString &text) const {
        if (m_runBtn) {
            m_runBtn->setText(text);
        }
    }

    void AsyncTaskWindow::onTaskFinished() {
        const QString msg = QString("任务完成！总数: %3, 成功: %1, 失败: %2")
                                .arg(m_totalTasks - m_errorTasks)
                                .arg(m_errorTasks)
                                .arg(m_totalTasks);

        if (m_errorTasks > 0) {
            m_logOutput->appendPlainText("失败任务列表：");
            for (const QString &detail : m_errorDetails) {
                m_logOutput->appendPlainText("  " + detail);
            }
            m_errorDetails.clear();
        }

        QMessageBox::information(this, QApplication::applicationName(), msg);
        m_totalTasks = m_finishedTasks = m_errorTasks = 0;
    }

    void AsyncTaskWindow::addFiles(const QStringList &paths) const {
        for (const QString &path : paths) {
            QFileInfo info(path);
            if (info.suffix().compare("wav", Qt::CaseInsensitive) == 0) {
                auto *item = new QListWidgetItem(info.fileName());
                item->setData(Qt::UserRole + 1, path);
                m_taskList->addItem(item);
            }
        }
    }

    void AsyncTaskWindow::addFolder(const QString &path) const {
        const QDir dir(path);
        QStringList files = dir.entryList(QDir::Files);
        QStringList fullPaths;
        for (const QString &file : files) {
            fullPaths << dir.absoluteFilePath(file);
        }
        addFiles(fullPaths);
    }

    void AsyncTaskWindow::createMenus() {
        m_addFileAction = new QAction("添加文件", this);
        m_addFolderAction = new QAction("添加文件夹", this);
        m_fileMenu = new QMenu("文件(&F)", this);
        m_fileMenu->addAction(m_addFileAction);
        m_fileMenu->addAction(m_addFolderAction);

        m_aboutAppAction = new QAction(QString("关于 %1").arg(QApplication::applicationName()), this);
        m_aboutQtAction = new QAction("关于 Qt", this);
        m_helpMenu = new QMenu("帮助(&H)", this);
        m_helpMenu->addAction(m_aboutAppAction);
        m_helpMenu->addAction(m_aboutQtAction);

        menuBar()->addMenu(m_fileMenu);
        menuBar()->addMenu(m_helpMenu);

        connect(m_addFileAction, &QAction::triggered, this, &AsyncTaskWindow::slot_addFile);
        connect(m_addFolderAction, &QAction::triggered, this, &AsyncTaskWindow::slot_addFolder);
        connect(m_aboutAppAction, &QAction::triggered, this, &AsyncTaskWindow::slot_aboutApp);
        connect(m_aboutQtAction, &QAction::triggered, this, &AsyncTaskWindow::slot_aboutQt);
    }

    void AsyncTaskWindow::setupCommonUI() {
        auto *central = new QWidget(this);
        auto *mainLayout = new QVBoxLayout(central);

        auto *splitLayout = new QHBoxLayout();

        auto *leftLayout = new QVBoxLayout();
        m_taskList = new QListWidget();
        m_taskList->setSelectionMode(QAbstractItemView::ExtendedSelection);
        leftLayout->addWidget(m_taskList, 1);

        auto *btnLayout = new QHBoxLayout();
        auto *removeBtn = new QPushButton("移除选中");
        auto *clearBtn = new QPushButton("清空列表");
        m_runBtn = new QPushButton("运行任务");
        btnLayout->addWidget(removeBtn);
        btnLayout->addWidget(clearBtn);
        btnLayout->addWidget(m_runBtn);
        leftLayout->addLayout(btnLayout);

        splitLayout->addLayout(leftLayout, 3);

        m_rightPanel = new QVBoxLayout();
        splitLayout->addLayout(m_rightPanel, 2);

        m_topLayout = new QVBoxLayout();
        mainLayout->addLayout(m_topLayout);
        mainLayout->addLayout(splitLayout);

        m_logOutput = new QPlainTextEdit();
        m_logOutput->setReadOnly(true);
        mainLayout->addWidget(m_logOutput);

        auto *progressLayout = new QHBoxLayout();
        progressLayout->addWidget(new QLabel("进度:"));
        m_progressBar = new QProgressBar();
        m_progressBar->setRange(0, 100);
        progressLayout->addWidget(m_progressBar);
        mainLayout->addLayout(progressLayout);

        setCentralWidget(central);

        connect(removeBtn, &QPushButton::clicked, this, &AsyncTaskWindow::slot_removeSelected);
        connect(clearBtn, &QPushButton::clicked, this, &AsyncTaskWindow::slot_clearList);
        connect(m_runBtn, &QPushButton::clicked, this, &AsyncTaskWindow::runTask);
    }

    void AsyncTaskWindow::dragEnterEvent(QDragEnterEvent *event) {
        bool accept = false;
        for (const QUrl &url : event->mimeData()->urls()) {
            if (url.isLocalFile()) {
                QFileInfo info(url.toLocalFile());
                if (info.isDir() || info.suffix().compare("wav", Qt::CaseInsensitive) == 0) {
                    accept = true;
                    break;
                }
            }
        }
        if (accept) {
            event->acceptProposedAction();
        }
    }

    void AsyncTaskWindow::dropEvent(QDropEvent *event) {
        QStringList wavFiles;
        for (const QUrl &url : event->mimeData()->urls()) {
            if (!url.isLocalFile())
                continue;
            QString path = url.toLocalFile();
            if (QFileInfo(path).isDir()) {
                addFolder(path);
            } else {
                wavFiles.append(path);
            }
        }
        if (!wavFiles.isEmpty()) {
            addFiles(wavFiles);
        }
        event->acceptProposedAction();
    }

    void AsyncTaskWindow::closeEvent(QCloseEvent *event) {
        event->accept();
    }

    void AsyncTaskWindow::slot_addFile() {
        const QStringList paths = QFileDialog::getOpenFileNames(
            this, "添加文件", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), "音频文件 (*.wav)");
        if (!paths.isEmpty()) {
            addFiles(paths);
        }
    }

    void AsyncTaskWindow::slot_addFolder() {
        const QString path = QFileDialog::getExistingDirectory(
            this, "添加文件夹", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        if (!path.isEmpty()) {
            addFolder(path);
        }
    }

    void AsyncTaskWindow::slot_removeSelected() const {
        qDeleteAll(m_taskList->selectedItems());
    }

    void AsyncTaskWindow::slot_clearList() const {
        m_taskList->clear();
    }

    void AsyncTaskWindow::slot_aboutApp() {
        QMessageBox::information(this, QApplication::applicationName(),
                                 QString("%1 版本 \n版权所有 © OpenVPI.").arg(QApplication::applicationName()));
    }

    void AsyncTaskWindow::slot_aboutQt() {
        QMessageBox::aboutQt(this);
    }

    void AsyncTaskWindow::slot_oneFinished(const QString &filename, const QString &msg) {
        m_finishedTasks++;
        m_progressBar->setValue(m_finishedTasks);

        if (!msg.isEmpty()) {
            m_logOutput->appendPlainText(filename + ": " + msg);
            m_logOutput->appendHtml("<hr>");
        }

        if (m_finishedTasks == m_totalTasks) {
            onTaskFinished();
        }
    }

    void AsyncTaskWindow::slot_oneFailed(const QString &filename, const QString &msg) {
        m_finishedTasks++;
        m_errorTasks++;
        m_errorDetails.append(filename + ": " + msg);
        m_progressBar->setValue(m_finishedTasks);
        m_logOutput->appendPlainText(filename + ": " + msg);

        if (m_finishedTasks == m_totalTasks) {
            onTaskFinished();
        }
    }

} // namespace AsyncTask