#include "MainWindow.h"


#include <QApplication>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QStatusBar>

#include "QMSystem.h"
#include <QStandardPaths>

#include "../util/FblThread.h"

namespace FBL {
    MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
        const QString modelFolder = QDir::cleanPath(
#ifdef Q_OS_MAC
            QApplication::applicationDirPath() + "/../Resources/fbl_model"
#else
            QApplication::applicationDirPath() + QDir::separator() + "fbl_model"
#endif
        );
        if (QDir(modelFolder).exists()) {
            const auto modelPath = modelFolder + QDir::separator() + "model.onnx";
            const auto configPath = modelFolder + QDir::separator() + "config.yaml";
            if (!QFile(modelPath).exists() || !QFile(configPath).exists())
                QMessageBox::information(
                    this, "Warning",
                    "Missing model.onnx or config.yaml, please read ReadMe.md and download the model again.");
            else
                m_fbl = new FBL(modelFolder);
        } else {
#ifdef Q_OS_MAC
            QMessageBox::information(
                this, "Warning",
                "Please read ReadMe.md and download asrModel to unzip to app bundle's Resources directory.");
#else
            QMessageBox::information(this, "Warning",
                                     "Please read ReadMe.md and download asrModel to unzip to the root directory.");
#endif
        }

        m_threadpool = new QThreadPool(this);
        m_threadpool->setMaxThreadCount(1);

        initStyleSheet();
        setAcceptDrops(true);

        // Init menus
        addFileAction = new QAction("Add File", this);
        addFolderAction = new QAction("Add Folder", this);

        fileMenu = new QMenu("File(&F)", this);
        fileMenu->addAction(addFileAction);
        fileMenu->addAction(addFolderAction);

        aboutAppAction = new QAction(QString("About %1").arg(QApplication::applicationName()), this);
        aboutQtAction = new QAction("About Qt", this);

        helpMenu = new QMenu("Help(&H)", this);
        helpMenu->addAction(aboutAppAction);
        helpMenu->addAction(aboutQtAction);

        const auto bar = menuBar();
        bar->addMenu(fileMenu);
        bar->addMenu(helpMenu);

        listLayout = new QHBoxLayout();

        taskList = new QListWidget();
        taskList->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

        rightLayout = new QVBoxLayout();
        rawTgLayout = new QHBoxLayout();
        outTgLayout = new QHBoxLayout();
        apThreshLayout = new QHBoxLayout();
        apDurLayout = new QHBoxLayout();
        spDurLayout = new QHBoxLayout();

        rawTgEdit = new QLineEdit(R"(D:\python\FoxBreatheLabeler\raw_textgrid)");
        const auto btnRawTg = new QPushButton("Open Folder");
        rawTgLayout->addWidget(rawTgEdit);
        rawTgLayout->addWidget(btnRawTg);

        outTgEdit = new QLineEdit(R"(D:\python\FoxBreatheLabeler\out_textgrid)");
        const auto btnOutTg = new QPushButton("Open Folder");
        outTgLayout->addWidget(outTgEdit);
        outTgLayout->addWidget(btnOutTg);

        const auto apThreshLabel = new QLabel("AP Threshold");
        ap_threshold = new QDoubleSpinBox();
        ap_threshold->setRange(0, 1);
        ap_threshold->setValue(0.4);
        ap_threshold->setSingleStep(0.01);
        apThreshLayout->addWidget(apThreshLabel);
        apThreshLayout->addWidget(ap_threshold);

        const auto apDurLabel = new QLabel("AP Duration");
        ap_dur = new QDoubleSpinBox();
        ap_dur->setRange(0.05, 5);
        ap_dur->setValue(0.08);
        ap_dur->setSingleStep(0.01);
        apDurLayout->addWidget(apDurLabel);
        apDurLayout->addWidget(ap_dur);

        const auto spDurLabel = new QLabel("SP Duration");
        sp_dur = new QDoubleSpinBox();
        sp_dur->setRange(0.01, 5);
        sp_dur->setValue(0.1);
        sp_dur->setSingleStep(0.01);
        spDurLayout->addWidget(spDurLabel);
        spDurLayout->addWidget(sp_dur);

        const auto rawTgLabel = new QLabel("Raw TextGrid Path:");
        const auto outTgLabel = new QLabel("Out TextGrid Path:");

        rightLayout->addWidget(rawTgLabel);
        rightLayout->addLayout(rawTgLayout);
        rightLayout->addWidget(outTgLabel);
        rightLayout->addLayout(outTgLayout);
        rightLayout->addLayout(apThreshLayout);
        rightLayout->addLayout(apDurLayout);
        rightLayout->addLayout(spDurLayout);
        rightLayout->addStretch(1);

        listLayout->addWidget(taskList, 3);
        listLayout->addLayout(rightLayout, 2);

        btnLayout = new QHBoxLayout();
        remove = new QPushButton("remove");
        clear = new QPushButton("clear");
        runFbl = new QPushButton("runFbl");
        if (!m_fbl)
            runFbl->setDisabled(true);
        btnLayout->addWidget(remove);
        btnLayout->addWidget(clear);
        btnLayout->addWidget(runFbl);

        out = new QPlainTextEdit();
        out->setReadOnly(true);

        progressLabel = new QLabel("progress:");
        progressBar = new QProgressBar();
        progressBar->setRange(0, 100);
        progressBar->setValue(0);

        progressLayout = new QHBoxLayout();
        progressLayout->addWidget(progressLabel);
        progressLayout->addWidget(progressBar);

        mainLayout = new QVBoxLayout();
        mainLayout->addLayout(listLayout);
        mainLayout->addLayout(btnLayout);
        mainLayout->addWidget(out);
        mainLayout->addLayout(progressLayout);

        mainWidget = new QWidget();
        mainWidget->setLayout(mainLayout);
        setCentralWidget(mainWidget);

        // Init signals
        connect(fileMenu, &QMenu::triggered, this, &MainWindow::_q_fileMenuTriggered);
        connect(helpMenu, &QMenu::triggered, this, &MainWindow::_q_helpMenuTriggered);

        connect(remove, &QPushButton::clicked, this, &MainWindow::slot_removeListItem);
        connect(clear, &QPushButton::clicked, this, &MainWindow::slot_clearTaskList);
        connect(runFbl, &QPushButton::clicked, this, &MainWindow::slot_runFbl);

        connect(btnRawTg, &QPushButton::clicked, this, &MainWindow::slot_rawTgPath);
        connect(btnOutTg, &QPushButton::clicked, this, &MainWindow::slot_outTgPath);

        resize(960, 720);
    }

    MainWindow::~MainWindow() = default;

    void MainWindow::addFiles(const QStringList &paths) const {
        for (const auto &path : paths) {
            const QFileInfo info(path);
            if (info.suffix() == "wav") {
                auto *item = new QListWidgetItem();
                item->setText(info.fileName());
                item->setData(Qt::ItemDataRole::UserRole + 1, path);
                taskList->addItem(item);
            }
        }
    }

    void MainWindow::addFolder(const QString &path) const {
        const QDir dir(path);
        QStringList files = dir.entryList(QDir::Files);
        QStringList absoluteFilePaths;
        for (const QString &file : files) {
            absoluteFilePaths << dir.absoluteFilePath(file);
        }
        addFiles(absoluteFilePaths);
    }

    void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
        auto urls = event->mimeData()->urls();
        bool has_wav = false;
        bool has_folder = false;
        for (const auto &url : urls) {
            if (url.isLocalFile()) {
                const auto path = url.toLocalFile();
                if (QFileInfo(path).isDir()) {
                    has_folder = true;
                } else {
                    const auto ext = QFileInfo(path).suffix();
                    if (ext.compare("wav", Qt::CaseInsensitive) == 0) {
                        has_wav = true;
                    }
                }
            }
        }
        if (has_wav || has_folder) {
            event->accept();
        }
    }

    void MainWindow::dropEvent(QDropEvent *event) {
        auto urls = event->mimeData()->urls();
        QStringList wavPaths;
        for (const auto &url : urls) {
            if (!url.isLocalFile()) {
                continue;
            }
            const auto path = url.toLocalFile();
            if (QFileInfo(path).isDir())
                addFolder(path);
            else
                wavPaths.append(path);
        }

        if (!wavPaths.isEmpty())
            addFiles(wavPaths);
    }

    void MainWindow::closeEvent(QCloseEvent *event) {
        // Quit
        event->accept();
    }

    void MainWindow::initStyleSheet() {
        // qss file: https://github.com/feiyangqingyun/QWidgetDemo/tree/master/ui/styledemo
        QFile file(QApplication::applicationDirPath() + "/qss/flatgray.css");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qApp->setStyleSheet(file.readAll());
        }
    }

    void MainWindow::_q_fileMenuTriggered(const QAction *action) {
        if (action == addFileAction) {
            const auto paths = QFileDialog::getOpenFileNames(
                this, "Add Files", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation), "*.wav");
            if (paths.isEmpty()) {
                return;
            }
            addFiles(paths);
        } else if (action == addFolderAction) {
            const QString path = QFileDialog::getExistingDirectory(
                this, "Add Folder", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
            if (path.isEmpty()) {
                return;
            }
            addFolder(path);
        }
    }

    void MainWindow::_q_helpMenuTriggered(const QAction *action) {
        if (action == aboutAppAction) {
            QMessageBox::information(
                this, QApplication::applicationName(),
                QString("%1 %2, Copyright OpenVPI.").arg(QApplication::applicationName(), APP_VERSION));
        } else if (action == aboutQtAction) {
            QMessageBox::aboutQt(this);
        }
    }

    void MainWindow::slot_removeListItem() const {
        auto itemList = taskList->selectedItems();
        for (const auto item : itemList) {
            delete item;
        }
    }

    void MainWindow::slot_clearTaskList() const {
        taskList->clear();
    }

    void MainWindow::slot_runFbl() {
        out->clear();
        m_threadpool->clear();

        const auto rawTgDir = rawTgEdit->text();
        if (!QDir(rawTgDir).exists()) {
            QMessageBox::information(nullptr, "Warning",
                                     "Raw TextGrid Path is empty or does not exist. Please set the output directory.");
            return;
        }

        const auto outTgDir = outTgEdit->text();
        if (!QDir(outTgDir).exists()) {
            QMessageBox::information(nullptr, "Warning",
                                     "Out TextGrid Path is empty or does not exist. Please set the output directory.");
            return;
        }

        m_workError = 0;
        m_workFinished = 0;
        m_workTotal = taskList->count();
        progressBar->setValue(0);
        progressBar->setMaximum(taskList->count());

        const auto ap_thresh = ap_threshold->value();
        const auto ap_duration = ap_dur->value();
        const auto sp_duration = sp_dur->value();

        for (int i = 0; i < taskList->count(); i++) {
            const auto item = taskList->item(i);
            const QString rawTgPath =
                rawTgDir + QDir::separator() + QFileInfo(item->text()).completeBaseName() + ".TextGrid";
            const QString outTgPath =
                outTgDir + QDir::separator() + QFileInfo(item->text()).completeBaseName() + ".TextGrid";

            const auto asrTread = new FblThread(m_fbl, item->text(), item->data(Qt::UserRole + 1).toString(), rawTgPath,
                                                outTgPath, ap_thresh, ap_duration, sp_duration);
            connect(asrTread, &FblThread::oneFailed, this, &MainWindow::slot_oneFailed);
            connect(asrTread, &FblThread::oneFinished, this, &MainWindow::slot_oneFinished);
            m_threadpool->start(asrTread);
        }
    }

    void MainWindow::slot_rawTgPath() {
        const QString path = QFileDialog::getExistingDirectory(
            this, "Raw TextGrid Folder Path", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        if (path.isEmpty()) {
            return;
        }
        rawTgEdit->setText(path);
    }

    void MainWindow::slot_outTgPath() {
        const QString path = QFileDialog::getExistingDirectory(
            this, "Out TextGrid Folder Path", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        if (path.isEmpty()) {
            return;
        }
        outTgEdit->setText(path);
    }

    void MainWindow::slot_oneFailed(const QString &filename, const QString &msg) {
        m_workFinished++;
        m_workError++;
        m_failIndex.append(filename + ": " + msg);
        progressBar->setValue(m_workFinished);

        out->appendPlainText(filename + ": " + msg);

        if (m_workFinished == m_workTotal) {
            slot_threadFinished();
        }
    }

    void MainWindow::slot_oneFinished(const QString &filename, const QString &msg) {
        m_workFinished++;
        progressBar->setValue(m_workFinished);

        if (!msg.isEmpty())
            out->appendPlainText(filename + ": " + msg);

        if (m_workFinished == m_workTotal) {
            slot_threadFinished();
        }
    }

    void MainWindow::slot_threadFinished() {
        const auto msg = QString("Fbl complete! Total: %3, Success: %1, Failed: %2")
                             .arg(m_workTotal - m_workError)
                             .arg(m_workError)
                             .arg(m_workTotal);
        if (m_workError > 0) {
            QString failSummary = "Failed tasks:\n";
            for (const QString &fileMsg : m_failIndex) {
                failSummary += "  " + fileMsg + "\n";
            }
            out->appendPlainText(failSummary);
            m_failIndex.clear();
        }
        QMessageBox::information(this, QApplication::applicationName(), msg);
        m_workFinished = 0;
        m_workError = 0;
        m_workTotal = 0;
    }
}