#include "MainWindow.h"

#include <cpp-pinyin/G2pglobal.h>

#include <QApplication>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QStatusBar>

#include <QStandardPaths>

#include "../util/AsrThread.h"
#include "../util/FaTread.h"

namespace LyricFA {
    MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_match(new MatchLyric()) {
        const QString modelFolder = QDir::cleanPath(
#ifdef Q_OS_MAC
            QApplication::applicationDirPath() + "/../Resources/model"
#else
            QApplication::applicationDirPath() + QDir::separator() + "model"
#endif
        );
        if (QDir(modelFolder).exists()) {
            const auto modelPath = modelFolder + QDir::separator() + "model.onnx";
            const auto vocabPath = modelFolder + QDir::separator() + "vocab.txt";
            if (!QFile(modelPath).exists() || !QFile(vocabPath).exists())
                QMessageBox::information(
                    this, "Warning",
                    "Missing model.onnx or vocab.txt, please read ReadMe.md and download the model again.");
            else
                m_asr = new Asr(modelFolder);
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

#ifdef Q_OS_MAC
        Pinyin::setDictionaryPath(QApplication::applicationDirPath().toUtf8().toStdString() + "/../Resources/dict");
#else
        Pinyin::setDictionaryPath(QApplication::applicationDirPath().toUtf8().toStdString() + "/dict");
#endif
        m_mandarin = QSharedPointer<Pinyin::Pinyin>(new Pinyin::Pinyin());

        m_threadpool = new QThreadPool(this);
        m_threadpool->setMaxThreadCount(1);

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
        labLayout = new QHBoxLayout();
        jsonLayout = new QHBoxLayout();
        lyricLayout = new QHBoxLayout();

        labEdit = new QLineEdit(R"(D:\python\LyricFA\test_outlab)");
        const auto btnLab = new QPushButton("Open Folder");
        labLayout->addWidget(labEdit);
        labLayout->addWidget(btnLab);

        jsonEdit = new QLineEdit(R"(D:\python\LyricFA\test_outjson)");
        const auto btnJson = new QPushButton("Open Folder");
        jsonLayout->addWidget(jsonEdit);
        jsonLayout->addWidget(btnJson);

        lyricEdit = new QLineEdit(R"(D:\python\LyricFA\lyrics)");
        const auto btnLyric = new QPushButton("Lyric Folder");
        lyricLayout->addWidget(lyricEdit);
        lyricLayout->addWidget(btnLyric);

        const auto labLabel = new QLabel("Lab Out Path:");
        const auto jsonLabel = new QLabel("Json Out Path:");
        const auto lyricLabel = new QLabel("Raw Lyric Path:");

        rightLayout->addWidget(labLabel);
        rightLayout->addLayout(labLayout);
        rightLayout->addWidget(jsonLabel);
        rightLayout->addLayout(jsonLayout);
        rightLayout->addWidget(lyricLabel);
        rightLayout->addLayout(lyricLayout);

        pinyinBox = new QCheckBox("ASR result saved as pinyin");
        rightLayout->addWidget(pinyinBox);

        rightLayout->addStretch(1);

        listLayout->addWidget(taskList, 3);
        listLayout->addLayout(rightLayout, 2);

        btnLayout = new QHBoxLayout();
        remove = new QPushButton("remove");
        clear = new QPushButton("clear");
        runAsr = new QPushButton("runAsr");
        if (!m_asr)
            runAsr->setDisabled(true);
        matchLyric = new QPushButton("match lyric");
        btnLayout->addWidget(remove);
        btnLayout->addWidget(clear);
        btnLayout->addWidget(runAsr);
        btnLayout->addWidget(matchLyric);

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
        connect(runAsr, &QPushButton::clicked, this, &MainWindow::slot_runAsr);
        connect(matchLyric, &QPushButton::clicked, this, &MainWindow::slot_matchLyric);

        connect(btnLab, &QPushButton::clicked, this, &MainWindow::slot_labPath);
        connect(btnJson, &QPushButton::clicked, this, &MainWindow::slot_jsonPath);
        connect(btnLyric, &QPushButton::clicked, this, &MainWindow::slot_lyricPath);

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

    void MainWindow::slot_runAsr() {
        out->clear();
        m_threadpool->clear();

        const auto labOutPath = labEdit->text();
        if (!QDir(labOutPath).exists()) {
            QMessageBox::information(nullptr, "Warning",
                                     "Lab Out Path is empty or does not exist. Please set the output directory.");
            return;
        }

        m_workError = 0;
        m_workFinished = 0;
        m_workTotal = taskList->count();
        progressBar->setValue(0);
        progressBar->setMaximum(taskList->count());

        const bool toPinyin = pinyinBox->isChecked();

        for (int i = 0; i < taskList->count(); i++) {
            const auto item = taskList->item(i);
            const QString labFilePath =
                labOutPath + QDir::separator() + QFileInfo(item->text()).completeBaseName() + ".lab";

            const auto asrTread = new AsrThread(m_asr, item->text(), item->data(Qt::UserRole + 1).toString(),
                                                labFilePath, toPinyin ? m_mandarin : nullptr);
            connect(asrTread, &AsrThread::oneFailed, this, &MainWindow::slot_oneFailed);
            connect(asrTread, &AsrThread::oneFinished, this, &MainWindow::slot_oneFinished);
            m_threadpool->start(asrTread);
        }
    }

    void MainWindow::slot_matchLyric() {
        const QString lyricFolder = lyricEdit->text();
        const QString labFolder = labEdit->text();
        const QString jsonFolder = jsonEdit->text();

        if (!QDir().exists()) {
            QMessageBox::information(nullptr, "Warning",
                                     "Raw Lyric Path is empty or does not exist. Please set the output directory.");
            return;
        }
        if (!QDir(labFolder).exists()) {
            QMessageBox::information(nullptr, "Warning",
                                     "Lab Out Path is empty or does not exist. Please set the output directory.");
            return;
        }
        if (!QDir(jsonFolder).exists()) {
            QMessageBox::information(nullptr, "Warning",
                                     "Json Out Path is empty or does not exist. Please set the output directory.");
            return;
        }

        out->clear();
        m_threadpool->clear();

        if (!QDir(jsonFolder).exists())
            qDebug() << QDir(jsonFolder).mkpath(jsonFolder);

        const QDir labDir(labFolder);
        QStringList labFiles = labDir.entryList();
        QStringList labPaths;
        for (const QString &file : labFiles) {
            if (QFileInfo(file).suffix() == "lab")
                labPaths << labDir.absoluteFilePath(file);
        }

        m_match->initLyric(lyricFolder);

        m_workError = 0;
        m_workFinished = 0;
        m_workTotal = labPaths.size();
        progressBar->setValue(0);
        progressBar->setMaximum(labPaths.size());

        for (const auto &labPath : labPaths) {
            const auto lab_name = QFileInfo(labPath).completeBaseName();
            const auto jsonPath = jsonFolder + QDir::separator() + lab_name + ".json";

            const auto faTread = new FaTread(m_match, lab_name, labPath, jsonPath);
            connect(faTread, &FaTread::oneFailed, this, &MainWindow::slot_oneFailed);
            connect(faTread, &FaTread::oneFinished, this, &MainWindow::slot_oneFinished);
            m_threadpool->start(faTread);
        }
    }

    void MainWindow::slot_labPath() {
        const QString path = QFileDialog::getExistingDirectory(
            this, "Lab Path", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        if (path.isEmpty()) {
            return;
        }
        labEdit->setText(path);
    }

    void MainWindow::slot_jsonPath() {
        const QString path = QFileDialog::getExistingDirectory(
            this, "Json Path", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        if (path.isEmpty()) {
            return;
        }
        jsonEdit->setText(path);
    }

    void MainWindow::slot_lyricPath() {
        const QString path = QFileDialog::getExistingDirectory(
            this, "Lyric Path", QStandardPaths::writableLocation(QStandardPaths::DesktopLocation));
        if (path.isEmpty()) {
            return;
        }
        lyricEdit->setText(path);
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
        const auto msg = QString("Asr complete! Total: %3, Success: %1, Failed: %2")
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