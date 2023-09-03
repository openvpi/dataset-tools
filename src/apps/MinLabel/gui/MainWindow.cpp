#include "MainWindow.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QJsonDocument>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QStatusBar>
#include <QStyledItemDelegate>
#include <QTime>

#include "QMSystem.h"
#include "qasglobal.h"

static QString audioFileToDsFile(const QString &filename) {
    QFileInfo info(filename);
    QString suffix = info.suffix().toLower();
    QString name = info.fileName();
    return info.absolutePath() + "/" + name.mid(0, name.size() - suffix.size() - 1) +
           (suffix != "wav" ? "_" + suffix : "") + ".lab";
}

static QString audioFileToTextFile(const QString &filename) {
    QFileInfo info(filename);
    QString suffix = info.suffix().toLower();
    QString name = info.fileName();
    return info.absolutePath() + "/" + name.mid(0, name.size() - suffix.size() - 1) +
           (suffix != "wav" ? "_" + suffix : "") + ".txt";
}

class CustomDelegate : public QStyledItemDelegate {
public:
    explicit CustomDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        const auto *model = dynamic_cast<const QFileSystemModel *>(index.model());
        if (!model) {
            return;
        }

        QFileInfo fileInfo(model->filePath(index));
        QString labFilePath = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + ".lab";

        QStyleOptionViewItem modifiedOption(option);
        if (QFile::exists(labFilePath) && fileInfo.isFile() && QFileInfo(labFilePath).size() > 0) {
            modifiedOption.palette.setColor(QPalette::Text, Qt::gray);
        } else {
            modifiedOption.palette.setColor(QPalette::Text, Qt::black);
        }

        QStyledItemDelegate::paint(painter, modifiedOption, index);
    }
};

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    notifyTimerId = 0;
    playing = false;

    setAcceptDrops(true);

    initStyleSheet();

    // Init menus
    browseAction = new QAction("Open Folder", this);
    browseAction->setShortcut(QKeySequence("Ctrl+O"));

    fileMenu = new QMenu("File(&F)", this);
    fileMenu->addAction(browseAction);

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

    aboutAppAction = new QAction(QString("About %1").arg(qApp->applicationName()), this);
    aboutQtAction = new QAction("About Qt", this);

    helpMenu = new QMenu("Help(&H)", this);
    helpMenu->addAction(aboutAppAction);
    helpMenu->addAction(aboutQtAction);

    auto bar = menuBar();
    bar->addMenu(fileMenu);
    bar->addMenu(editMenu);
    bar->addMenu(playMenu);
    bar->addMenu(helpMenu);

    // Status bar
    checkPreserveText = new QCheckBox("Preserve text", this);

    progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    auto progressLabel = new QLabel("progress:");

    auto status = statusBar();
    status->setContentsMargins(20, 2, 20, 2);
    status->setSizeGripEnabled(false);
    status->addWidget(checkPreserveText);
    status->addPermanentWidget(progressLabel);
    status->addPermanentWidget(progressBar);

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

    QString keyConfPath = qApp->applicationDirPath() + "/minlabel_config.json";
    QJsonDocument cfgDoc;
    try {
        if (qApp->arguments().contains("--reset-keys")) {
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
            cfg.next = nextAction->shortcut().toString();
            cfg.prev = prevAction->shortcut().toString();
            cfg.play = playAction->shortcut().toString();
            cfg.preserveText = false;
            cfg.rootDir = dirname;
            applyConfig();
            file.write(QJsonDocument(qAsClassToJson(cfg)).toJson());
            file.close();
        } else {
            QMessageBox::critical(this, qApp->applicationName(), "Failed to create config file.");
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

void MainWindow::openFile(const QString &filename) {
    QString labContent, txtContent;

    QString labFilePath = audioFileToDsFile(filename);
    QFile labFile(labFilePath);
    if (labFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        labContent = QString::fromUtf8(labFile.readAll());
    }
    textWidget->contentText->setPlainText(labContent);

    if (checkPreserveText->isChecked()) {
        QString txtFilePath = audioFileToTextFile(filename);
        QFile txtFile(txtFilePath);
        if (txtFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            txtContent = QString::fromUtf8(txtFile.readAll());
        }
        textWidget->wordsText->setText(txtContent);
    }

    playerWidget->openFile(filename);
}

void MainWindow::saveFile(const QString &filename) {
    QString labFilePath = audioFileToDsFile(filename);

    QString labContent = textWidget->contentText->toPlainText();
    if (labContent.isEmpty() && !QMFs::isFileExist(labFilePath)) {
        return;
    }

    QFile labFile(labFilePath);
    if (!labFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, qApp->applicationName(),
                              QString("Failed to write to file %1").arg(QMFs::PathFindFileName(labFilePath)));
        ::exit(-1);
    }

    QTextStream labIn(&labFile);
    labIn.setCodec(QTextCodec::codecForName("UTF-8"));
    labIn << labContent;

    // Preserve text
    if (checkPreserveText->isChecked()) {
        QString txtFilePath = audioFileToTextFile(filename);
        QString txtContent = textWidget->wordsText->text();
        if (txtContent.isEmpty() && !QMFs::isFileExist(txtFilePath)) {
            return;
        }

        QFile txtFile(txtFilePath);
        if (!txtFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, qApp->applicationName(),
                                  QString("Failed to write to file %1").arg(QMFs::PathFindFileName(txtFilePath)));
            ::exit(-1);
        }

        QTextStream txtIn(&txtFile);
        txtIn.setCodec(QTextCodec::codecForName("UTF-8"));
        txtIn << txtContent;
    }

    labFile.close();
}

void MainWindow::reloadWindowTitle() {
    setWindowTitle(dirname.isEmpty()
                       ? qApp->applicationName()
                       : QString("%1 - %2").arg(qApp->applicationName(),
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
            }
        }
        if (ok) {
            e->acceptProposedAction();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    // Pull and save config
    QString keyConfPath = qApp->applicationDirPath() + "/minlabel_config.json";
    QFile file(keyConfPath);

    cfg.preserveText = checkPreserveText->isChecked();

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
    prevAction->setShortcut(QKeySequence(cfg.prev));
    nextAction->setShortcut(QKeySequence(cfg.next));
    playAction->setShortcut(QKeySequence(cfg.play));
    checkPreserveText->setChecked(cfg.preserveText);
    dirname = cfg.rootDir;
    if (dirname.isEmpty()) {
        return;
    }
    openDirectory(dirname);
    emit(_q_updateProgress());
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
        emit(_q_updateProgress());
    }
    reloadWindowTitle();
}

void MainWindow::_q_editMenuTriggered(QAction *action) {
    if (action == prevAction) {
        QKeyEvent e(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
        QApplication::sendEvent(treeView, &e);
    } else if (action == nextAction) {
        QKeyEvent e(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
        QApplication::sendEvent(treeView, &e);
    }
}

void MainWindow::_q_playMenuTriggered(QAction *action) {
    if (action == playAction) {
        playerWidget->setPlaying(!playerWidget->isPlaying());
    }
}

void MainWindow::_q_helpMenuTriggered(QAction *action) {
    if (action == aboutAppAction) {
        QMessageBox::information(this, qApp->applicationName(),
                                 QString("%1 %2, Copyright OpenVPI.").arg(qApp->applicationName(), APP_VERSION));
    } else if (action == aboutQtAction) {
        QMessageBox::aboutQt(this);
    }
}

void MainWindow::_q_updateProgress() {
    QDir directory(dirname);
    QFileInfoList fileInfoList = directory.entryInfoList(QDir::Files);

    int count = 0;
    foreach (const QFileInfo &fileInfo, fileInfoList) {
        if (fileInfo.suffix() == "lab" && fileInfo.size() > 0) {
            count++;
        }
    }

    int totalRowCount = static_cast<int>(fsModel->rootDirectory().count());
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
