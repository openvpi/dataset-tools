#include "MainWindow.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPluginLoader>
#include <QStatusBar>
#include <QTime>


#include <QJsonObject>

// #include "Common/CodecArguments.h"
// #include "Common/SampleFormat.h"

// #include "MathHelper.h"
#include "QMSystem.h"
#include "qasglobal.h"

// https://iconduck.com/icons

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

    auto status = statusBar();
    status->addWidget(checkPreserveText);

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

        QJsonParseError err;
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

void MainWindow::openDirectory(const QString &dirname) {
    fsModel->setRootPath(dirname);
    treeView->setRootIndex(fsModel->index(dirname));
}

void MainWindow::openFile(const QString &filename) {
    QString labContent, txtContent;

    QString labFile = audioFileToDsFile(filename);
    QFile file(labFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        labContent = QString::fromUtf8(file.readAll());
    }
    textWidget->contentText->setPlainText(labContent);

    if(checkPreserveText->isChecked()) {
        QString txtFile = audioFileToTextFile(filename);
        QFile file(txtFile);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            txtContent = QString::fromUtf8(file.readAll());
        }
        textWidget->wordsText->setText(txtContent);
    }

    playerWidget->openFile(filename);
}

void MainWindow::saveFile(const QString &filename) {
    QString labFile = audioFileToDsFile(filename);

    QString labContent = textWidget->contentText->toPlainText();
    if (labContent.isEmpty() && !QMFs::isFileExist(labFile)) {
        return;
    }

    QFile file(labFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, qApp->applicationName(),
                              QString("Failed to write to file %1").arg(QMFs::PathFindFileName(labFile)));
        ::exit(-1);
    }

    QTextStream in(&file);
    in.setCodec(QTextCodec::codecForName("UTF-8"));
    in << labContent;

    // Preserve text
    if (checkPreserveText->isChecked()) {
        QString txtFile = audioFileToTextFile(filename);
        QString txtContent = textWidget->wordsText->text();
        if (txtContent.isEmpty() && !QMFs::isFileExist(txtFile)) {
            return;
        }

        QFile file(txtFile);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, qApp->applicationName(),
                                  QString("Failed to write to file %1").arg(QMFs::PathFindFileName(txtFile)));
            ::exit(-1);
        }

        QTextStream in(&file);
        in.setCodec(QTextCodec::codecForName("UTF-8"));
        in << txtContent;
    }

    file.close();
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
        for (auto it = urls.begin(); it != urls.end(); ++it) {
            if (it->isLocalFile()) {
                filenames.append(QMFs::removeTailSlashes(it->toLocalFile()));
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
        for (auto it = urls.begin(); it != urls.end(); ++it) {
            if (it->isLocalFile()) {
                filenames.append(QMFs::removeTailSlashes(it->toLocalFile()));
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
    QFile file(":/res/app.qss");
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
                                 QString("%1 %2, Copyright OpenVPI.").arg(qApp->applicationName(), APP_VERSON));
    } else if (action == aboutQtAction) {
        QMessageBox::aboutQt(this);
    }
}

void MainWindow::_q_treeCurrentChanged(const QModelIndex &current, const QModelIndex &previous) {
    Q_UNUSED(previous);

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
