#include "MainWindow.h"

#include "ExportDialog.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QStatusBar>
#include <QStyledItemDelegate>
#include <utility>

#include "QMSystem.h"

class CustomDelegate final : public QStyledItemDelegate {
public:
    explicit CustomDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        const auto *model = dynamic_cast<const QFileSystemModel *>(index.model());
        if (!model) {
            return;
        }

        const QFileInfo fileInfo(model->filePath(index));
        const QString jsonFilePath = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + ".json";

        QStyleOptionViewItem modifiedOption(option);
        if (QFile::exists(jsonFilePath) && fileInfo.isFile() && QFileInfo(jsonFilePath).size() > 0) {
            modifiedOption.palette.setColor(QPalette::Text, Qt::gray);
        } else {
            modifiedOption.palette.setColor(QPalette::Text, Qt::black);
        }

        QStyledItemDelegate::paint(painter, modifiedOption, index);
    }
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    playing = false;

    setAcceptDrops(true);

    initStyleSheet();

    // Init menus
    browseAction = new QAction("Open Folder", this);
    browseAction->setShortcut(QKeySequence("Ctrl+O"));

    covertAction = new QAction("Covert lab to project file", this);

    exportAction = new QAction("Export", this);
    exportAction->setShortcut(QKeySequence("Ctrl+E"));

    fileMenu = new QMenu("File(&F)", this);
    fileMenu->addAction(browseAction);
    fileMenu->addAction(exportAction);
    fileMenu->addAction(covertAction);

    nextAction = new QAction("Next file", this);
    nextAction->setShortcut(QKeySequence::MoveToNextPage);

    prevAction = new QAction("Previous file", this);
    prevAction->setShortcut(QKeySequence::MoveToPreviousPage);

    editMenu = new QMenu("Edit(&E)", this);
    editMenu->addAction(nextAction);
    editMenu->addAction(prevAction);

    playAction = new QAction("Play/Stop", this);
    playAction->setShortcut(QKeySequence("F5"));

    playMenu = new QMenu("Playback(&P)", this);
    playMenu->addAction(playAction);

    aboutAppAction = new QAction(QString("About %1").arg(QApplication::applicationName()), this);
    aboutQtAction = new QAction("About Qt", this);

    helpMenu = new QMenu("Help(&H)", this);
    helpMenu->addAction(aboutAppAction);
    helpMenu->addAction(aboutQtAction);

    auto bar = menuBar();
    bar->addMenu(fileMenu);
    bar->addMenu(editMenu);
    bar->addMenu(playMenu);
    bar->addMenu(helpMenu);

    progressLabel = new QLabel("progress:");
    progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);

    progressLayout = new QHBoxLayout();
    progressLayout->addWidget(progressLabel);
    progressLayout->addWidget(progressBar);

    // Init widgets
    playerWidget = new PlayWidget();
    playerWidget->setObjectName("play-widget");
    playerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    textWidget = new TextWidget();
    textWidget->setObjectName("text-widget");

    fsModel = new QFileSystemModel();
    fsModel->setParent(this);
    fsModel->setNameFilters({"*.wav", "*.mp3", "*.m4a", "*.flac"});
    fsModel->setNameFilterDisables(false);

    treeView = new QTreeView();
    treeView->setModel(fsModel);
    treeView->setItemDelegate(new CustomDelegate(treeView));

    {
        QSet<QString> names{"Name", "Size"};
        for (int i = 0; i < fsModel->columnCount(); ++i) {
            if (!names.contains(fsModel->headerData(i, Qt::Horizontal).toString())) {
                treeView->hideColumn(i);
            }
        }
    }

    rightLayout = new QVBoxLayout();
    rightLayout->addWidget(playerWidget);
    rightLayout->addWidget(textWidget);
    rightLayout->addLayout(progressLayout);

    rightWidget = new QWidget();
    rightWidget->setLayout(rightLayout);

    mainSplitter = new QSplitter();
    mainSplitter->setObjectName("central-widget");
    mainSplitter->addWidget(treeView);
    mainSplitter->addWidget(rightWidget);
    setCentralWidget(mainSplitter);

    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);

    // Init signals
    connect(fileMenu, &QMenu::triggered, this, &MainWindow::_q_fileMenuTriggered);
    connect(editMenu, &QMenu::triggered, this, &MainWindow::_q_editMenuTriggered);
    connect(playMenu, &QMenu::triggered, this, &MainWindow::_q_playMenuTriggered);
    connect(helpMenu, &QMenu::triggered, this, &MainWindow::_q_helpMenuTriggered);

    connect(treeView->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::_q_treeCurrentChanged);
    connect(treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &MainWindow::_q_updateProgress);

    reloadWindowTitle();
    resize(1280, 720);

    QString keyConfPath = QApplication::applicationDirPath() + "/minlabel_config.json";
    QJsonDocument cfgDoc;
    try {
        if (QApplication::arguments().contains("--reset-keys")) {
            throw std::exception();
        }

        QFile file(keyConfPath);
        if (!file.open(QIODevice::ReadOnly)) {
            throw std::exception();
        }

        QJsonParseError err{};
        cfgDoc = QJsonDocument::fromJson(file.readAll(), &err);

        file.close();

        if (err.error != QJsonParseError::NoError || !cfgDoc.isObject()) {
            throw std::exception();
        }

        QAS::JsonStream stream(cfgDoc.object());
        stream >> cfg;
        applyConfig();
    } catch (...) {
        QFile file(keyConfPath);
        if (file.open(QIODevice::WriteOnly)) {
            cfg.open = browseAction->shortcut().toString();
            cfg.exportAudio = exportAction->shortcut().toString();
            cfg.next = nextAction->shortcut().toString();
            cfg.prev = prevAction->shortcut().toString();
            cfg.play = playAction->shortcut().toString();
            cfg.rootDir = dirname;
            applyConfig();
            file.write(QJsonDocument(qAsClassToJson(cfg)).toJson());
            file.close();
        } else {
            QMessageBox::critical(this, QApplication::applicationName(), "Failed to create config file.");
        }
    }
}

MainWindow::~MainWindow() {
    if (QMFs::isFileExist(lastFile)) {
        saveFile(lastFile);
    }
}

void MainWindow::openDirectory(const QString &dirName) {
    fsModel->setRootPath(dirName);
    treeView->setRootIndex(fsModel->index(dirName));
}

void MainWindow::openFile(const QString &filename) const {
    QString labContent, txtContent;

    QString jsonFilePath = audioToOtherSuffix(filename, "json");
    QJsonObject readData;
    if (readJsonFile(jsonFilePath, readData)) {
        txtContent = readData.contains("raw_text") ? readData["raw_text"].toString() : "";
        labContent = readData.contains("lab") ? readData["lab"].toString() : "";
    }
    textWidget->contentText->setPlainText(labContent);
    textWidget->wordsText->setText(txtContent);

    playerWidget->openFile(filename);
}

void MainWindow::saveFile(const QString &filename) {
    QString txtContent = textWidget->wordsText->text();
    QString labContent = textWidget->contentText->toPlainText();
    QString withoutTone = "";
    if (!labContent.isEmpty() && labContent.contains(" ")) {
        QStringList inputList = labContent.split(" ");
        for (QString &item : inputList) {
            item.remove(QRegExp("[^a-z]"));
        }
        withoutTone = inputList.join(" ");
    }


    QString jsonFilePath = audioToOtherSuffix(filename, "json");

    if (labContent.isEmpty() && txtContent.isEmpty() && !QMFs::isFileExist(jsonFilePath)) {
        return;
    }

    QFile jsonFile(jsonFilePath);
    QJsonObject writeData;
    writeData["lab"] = labContent.replace(QRegExp("\\s+"), " ");
    writeData["raw_text"] = txtContent;
    writeData["lab_without_tone"] = withoutTone.replace(QRegExp("\\s+"), " ");
    writeData["isCheck"] = true;

    if (!writeJsonFile(jsonFilePath, writeData)) {
        QMessageBox::critical(this, QApplication::applicationName(),
                              QString("Failed to write to file %1").arg(QMFs::PathFindFileName(jsonFilePath)));
        ::exit(-1);
    }

    QString labFilePath = audioToOtherSuffix(filename, "lab");

    if (labContent.isEmpty() && !QMFs::isFileExist(labFilePath)) {
        return;
    }

    QFile labFile(labFilePath);
    if (!labFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, QApplication::applicationName(),
                              QString("Failed to write to file %1").arg(QMFs::PathFindFileName(labFilePath)));
        ::exit(-1);
    }

    QTextStream labIn(&labFile);
    labIn.setCodec(QTextCodec::codecForName("UTF-8"));
    labIn << labContent;
    labFile.close();
}

void MainWindow::reloadWindowTitle() {
    setWindowTitle(dirname.isEmpty()
                       ? QApplication::applicationName()
                       : QString("%1 - %2").arg(QApplication::applicationName(),
                                                QDir::toNativeSeparators(QMFs::PathFindFileName(dirname))));
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    auto e = static_cast<QDragEnterEvent *>(event);
    const QMimeData *mime = e->mimeData();
    if (mime->hasUrls()) {
        auto urls = mime->urls();
        QStringList filenames;
        for (auto &url : urls) {
            if (url.isLocalFile()) {
                filenames.append(QMFs::removeTailSlashes(url.toLocalFile()));
            }
        }
        bool ok = false;
        if (filenames.size() == 1) {
            QString filename = filenames.front();
            if (QMFs::isDirExist(filename)) {
                ok = true;
            }
        }
        if (ok) {
            e->acceptProposedAction();
        }
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    auto e = static_cast<QDropEvent *>(event);
    const QMimeData *mime = e->mimeData();
    if (mime->hasUrls()) {
        auto urls = mime->urls();
        QStringList filenames;
        for (auto &url : urls) {
            if (url.isLocalFile()) {
                filenames.append(QMFs::removeTailSlashes(url.toLocalFile()));
            }
        }
        bool ok = false;
        if (filenames.size() == 1) {
            QString filename = filenames.front();
            if (QMFs::isDirExist(filename)) {
                ok = true;
                openDirectory(filename);

                dirname = filename;
                cfg.rootDir = dirname;
                _q_updateProgress();
            }
        }
        if (ok) {
            e->acceptProposedAction();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // Pull and save config
    QString keyConfPath = QApplication::applicationDirPath() + "/minlabel_config.json";
    QFile file(keyConfPath);

    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(qAsClassToJson(cfg)).toJson());
    }

    // Quit
    event->accept();
}

void MainWindow::initStyleSheet() {
    // qss file: https://github.com/feiyangqingyun/QWidgetDemo/tree/master/ui/styledemo
    QFile file(":/qss/flatgray.css");
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qApp->setStyleSheet(file.readAll());
    }
}

void MainWindow::applyConfig() {
    browseAction->setShortcut(QKeySequence(cfg.open));
    exportAction->setShortcut(QKeySequence(cfg.exportAudio));
    prevAction->setShortcut(QKeySequence(cfg.prev));
    nextAction->setShortcut(QKeySequence(cfg.next));
    playAction->setShortcut(QKeySequence(cfg.play));
    dirname = cfg.rootDir;
    if (dirname.isEmpty()) {
        return;
    }
    openDirectory(dirname);
    _q_updateProgress();
    reloadWindowTitle();
}

void MainWindow::_q_fileMenuTriggered(QAction *action) {
    if (action == browseAction) {
        playerWidget->setPlaying(false);

        QString path = QFileDialog::getExistingDirectory(this, "Open Folder", QFileInfo(dirname).absolutePath());
        if (path.isEmpty()) {
            return;
        }
        openDirectory(path);

        dirname = path;
        cfg.rootDir = dirname;
        _q_updateProgress();
    } else if (action == covertAction) {
        playerWidget->setPlaying(false);
        int choice = QMessageBox::question(this, "Covert lab to project file.",
                                           "Do you want to covert lab to project file in target folder?",
                                           QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (choice == QMessageBox::Yes) {
            QString path =
                QFileDialog::getExistingDirectory(this, "Open Lab Folder", QFileInfo(dirname).absolutePath());
            if (path.isEmpty()) {
                return;
            }
            labToJson(path);
        }
    } else if (action == exportAction) {
        playerWidget->setPlaying(false);
        ExportDialog dialog(this);

        if (dialog.exec() == QDialog::Accepted) {
            exportAudio(dialog.exportInfo);
        }
    }
    reloadWindowTitle();
}

void MainWindow::_q_editMenuTriggered(const QAction *action) const {
    if (action == prevAction) {
        QKeyEvent e(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
        QApplication::sendEvent(treeView, &e);
    } else if (action == nextAction) {
        QKeyEvent e(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
        QApplication::sendEvent(treeView, &e);
    }
}

void MainWindow::_q_playMenuTriggered(const QAction *action) const {
    if (action == playAction) {
        playerWidget->setPlaying(!playerWidget->isPlaying());
    }
}

void MainWindow::_q_helpMenuTriggered(QAction *action) {
    if (action == aboutAppAction) {
        QMessageBox::information(
            this, QApplication::applicationName(),
            QString("%1 %2, Copyright OpenVPI.").arg(QApplication::applicationName(), APP_VERSION));
    } else if (action == aboutQtAction) {
        QMessageBox::aboutQt(this);
    }
}

void MainWindow::_q_updateProgress() const {
    const int count = jsonCount(dirname);
    const int totalRowCount = static_cast<int>(fsModel->rootDirectory().count());
    double progress = 0.0;
    if (totalRowCount > 0) {
        progress = (static_cast<double>(count) / totalRowCount) * 100.0;
    }
    progressBar->setValue(static_cast<int>(progress));
}

void MainWindow::_q_treeCurrentChanged(const QModelIndex &current) {
    QFileInfo info = fsModel->fileInfo(current);
    if (info.isFile()) {
        if (QMFs::isFileExist(lastFile)) {
            saveFile(lastFile);
        }
        lastFile = info.absoluteFilePath();
        textWidget->wordsText->clear();
        openFile(lastFile);
    }
}

void MainWindow::exportAudio(ExportInfo &exportInfo) {
    const int count = jsonCount(dirname);
    const int totalRowCount = static_cast<int>(fsModel->rootDirectory().count());
    if (totalRowCount != count) {
        const QMessageBox::StandardButton reply =
            QMessageBox::question(this, QApplication::applicationName(),
                                  QString("%1 file are not labeled, continue?").arg(totalRowCount - count),
                                  QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            return;
        }
    }

    mkdir(exportInfo);
    QList<CopyInfo> copyList = mkCopylist(dirname, exportInfo.outputDir + "/" + exportInfo.folderName);
    if (copyFile(copyList, exportInfo)) {
        QMessageBox::information(this, QApplication::applicationName(), QString("Successfully exported files."));
    } else {
        QMessageBox::critical(this, QApplication::applicationName(), QString("Failed to export files."));
    }
}
void MainWindow::labToJson(const QString &dirName) {
    const QDir directory(dirName);
    QFileInfoList fileInfoList = directory.entryInfoList(QDir::Files);

    int count = 0;
    foreach (const QFileInfo &fileInfo, fileInfoList) {
        QString currentFilePath = fsModel->fileInfo(treeView->currentIndex()).absoluteFilePath();
        QString labFilePath = fileInfo.absoluteFilePath();
        QString suffix = fileInfo.suffix().toLower();
        QString name = fileInfo.fileName();
        QString jsonFilePath = fileInfo.absolutePath() + "/" + name.mid(0, name.size() - suffix.size() - 1) + ".json";
        if (fileInfo.suffix() == "lab" && !QMFs::isFileExist(jsonFilePath)) {
            QFile file(labFilePath);
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QString labContent;
                labContent = QString::fromUtf8(file.readAll());
                const QString txtContent = labContent;

                if (audioToOtherSuffix(currentFilePath, "lab") == labFilePath) {
                    textWidget->contentText->setPlainText(labContent);
                    textWidget->wordsText->setText(labContent);
                } else {
                    QString withoutTone = "";
                    if (!labContent.isEmpty() && labContent.contains(" ")) {
                        QStringList inputList = labContent.split(" ");
                        for (QString &item : inputList) {
                            item.remove(QRegExp("[^a-z]"));
                        }
                        withoutTone = inputList.join(" ");
                    }

                    QFile jsonFile(jsonFilePath);
                    QJsonObject writeData;
                    writeData["lab"] = labContent.replace(QRegExp("\\s+"), " ");
                    writeData["raw_text"] = txtContent;
                    writeData["lab_without_tone"] = withoutTone.replace(QRegExp("\\s+"), " ");

                    if (!writeJsonFile(jsonFilePath, writeData)) {
                        QMessageBox::critical(
                            this, QApplication::applicationName(),
                            QString("Failed to write to file %1").arg(QMFs::PathFindFileName(jsonFilePath)));
                        ::exit(-1);
                    }
                }
                count++;
            }
        }
    }
    _q_updateProgress();
    QMessageBox::information(this, QApplication::applicationName(),
                             QString("Convert %1 lab files to current project file.").arg(count));
}
