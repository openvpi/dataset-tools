#include "MainWindow.h"
#include "SlurCutterCfg.h"

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
#include <QTime>

#include <QJsonArray>
#include <qasglobal.h>

// #include "Common/CodecArguments.h"
// #include "Common/SampleFormat.h"

// #include "MathHelper.h"
#include "QMSystem.h"

// https://iconduck.com/icons

static QString audioFileToDsFile(const QString &filename) {
    QFileInfo info(filename);
    QString suffix = info.suffix().toLower();
    QString name = info.fileName();
    return info.absolutePath() + "/" + name.mid(0, name.size() - suffix.size() - 1) +
           (suffix != "wav" ? "_" + suffix : "") + ".ds";
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

    // Init widgets
    playerWidget = new PlayWidget();
    playerWidget->setObjectName("play-widget");
    playerWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    f0Widget = new F0Widget();
    f0Widget->setObjectName("f0-widget");

    sentenceWidget = new QListWidget();
    sentenceWidget->setObjectName("sentence-widget");

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
    rightLayout->addWidget(f0Widget);

    rightWidget = new QWidget();
    rightWidget->setLayout(rightLayout);

    fsSentencesSplitter = new QSplitter();
    fsSentencesSplitter->setObjectName("fs-sentences-splitter");
    fsSentencesSplitter->addWidget(treeView);
    fsSentencesSplitter->addWidget(sentenceWidget);

    mainSplitter = new QSplitter();
    mainSplitter->setObjectName("central-widget");
    mainSplitter->addWidget(fsSentencesSplitter);
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
    connect(sentenceWidget, &QListWidget::currentRowChanged, this, &MainWindow::_q_sentenceChanged);
    connect(f0Widget, &F0Widget::requestReloadSentence, this, &MainWindow::reloadDsSentenceRequested);

    connect(playerWidget, &PlayWidget::playheadChanged, f0Widget, &F0Widget::setPlayheadPos);

    reloadWindowTitle();
    resize(1280, 720);

    QString keyConfPath = qApp->applicationDirPath() + "/slurcutter_config.json";
    QFile rfile(keyConfPath);
    if (!rfile.open(QIODevice::ReadOnly) || qApp->arguments().contains("--reset-keys")) {
        QFile file(keyConfPath);
        if (file.open(QIODevice::WriteOnly)) {
            SlurCutterCfg cfg;
            QAS::JsonStream stream;
            cfg.open = browseAction->shortcut().toString();
            cfg.next = nextAction->shortcut().toString();
            cfg.prev = prevAction->shortcut().toString();
            cfg.play = playAction->shortcut().toString();
            file.write(QJsonDocument(qAsClassToJson(cfg)).toJson());
        }
    }
    {
        // Read actions
        rfile.close();
        if (rfile.open(QIODevice::ReadOnly)) {
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(rfile.readAll(), &err);

            rfile.close();

            QAS::JsonStream stream(doc.object());
            if (err.error == QJsonParseError::NoError && doc.isObject()) {
                stream >> cfg;
            }
        }

        if (!cfg.open.isEmpty()) {
            browseAction->setShortcut(QKeySequence(cfg.open));
        }
        if (!cfg.next.isEmpty()) {
            nextAction->setShortcut(QKeySequence(cfg.next));
        }
        if (!cfg.prev.isEmpty()) {
            prevAction->setShortcut(QKeySequence(cfg.prev));
        }
        if (!cfg.play.isEmpty()) {
            playAction->setShortcut(QKeySequence(cfg.play));
        }

        f0Widget->loadConfig(cfg);
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
    QString content;

    f0Widget->clear();
    sentenceWidget->clear();
    dsContent.clear();

    playerWidget->openFile(filename);

    QString labFile = audioFileToDsFile(filename);
    QFile file(labFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        content = QString::fromUtf8(file.readAll());
        loadDsContent(content);
    } else {
        f0Widget->setErrorStatusText("No DS file can be opened");
    }

    // f0Widget->contentText->setPlainText(content);
}

bool MainWindow::saveFile(const QString &filename) {
    QJsonArray docArr;
    foreach (auto &i, dsContent) {
        docArr.append(i);
    }
    QJsonDocument doc(docArr);
    auto labFile = audioFileToDsFile(filename);
    QFile file(labFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        return true;
    } else {
        QMessageBox::critical(this, "Save error",
                              QString("Cannot open %1 to write!\n\n"
                                      "File switch was not performed. If you were to address the save problem,\n"
                                      "later you can switch file again and saving will be attempted again.")
                                  .arg(file.fileName()));
        return false;
    }
}

void MainWindow::pullEditedMidi() {
    if (currentRow < 0 || f0Widget->empty())
        return;
    auto &currentSentence = dsContent[currentRow];
    auto editedSentence = f0Widget->getSavedDsStrings();

    currentSentence["note_seq"] = editedSentence.note_seq;
    currentSentence["note_slur"] = editedSentence.note_slur;
    currentSentence["note_dur"] = editedSentence.note_dur;
}

void MainWindow::switchFile(bool next) {
    fileSwitchDirection = next;
    if (next) {
        QKeyEvent e(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
        QApplication::sendEvent(treeView, &e);
    } else {
        QKeyEvent e(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
        QApplication::sendEvent(treeView, &e);
    }
}

void MainWindow::switchSentence(bool next) {
    if (next) {
        if (currentRow == sentenceWidget->count() - 1) {
            switchFile(next);
        } else {
            QKeyEvent e(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
            QApplication::sendEvent(sentenceWidget, &e);
        }
    } else {
        if (currentRow == 0) {
            switchFile(next);
        } else {
            QKeyEvent e(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
            QApplication::sendEvent(sentenceWidget, &e);
        }
    }
}

void MainWindow::loadDsContent(const QString &content) {
    // Parse as JSON array
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), &err);

    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        f0Widget->setErrorStatusText(QString("Failed to parse JSON: %1").arg(err.errorString()));
        return;
    }

    QJsonArray arr = doc.array();
    if (arr.size() == 0) {
        f0Widget->setErrorStatusText("Empty JSON array");
        return;
    }

    // Import sentences
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        QJsonObject obj = it->toObject();
        if (obj.isEmpty()) {
            continue;
        }
        QString text = obj.value("text").toString();
        if (text.isEmpty()) {
            continue;
        }
        QString noteDuration = obj.value("note_dur").toString();
        if (noteDuration.isEmpty()) {
            continue;
        }
        auto noteDur = noteDuration.split(' ', QString::SkipEmptyParts);
        double offset = obj.value("offset").toDouble(-1);
        if (offset == -1) {
            continue;
        }

        double dur = 0.0;
        foreach (QString durStr, noteDur) {
            dur += durStr.toDouble();
        }

        auto item = new QListWidgetItem(QString("%1-%2").arg(offset, 0, 'f', 2, QChar('0')).arg(text.replace(' ', "")));
        item->setData(Qt::UserRole + 1, offset);
        item->setData(Qt::UserRole + 2, dur);
        sentenceWidget->addItem(item);

        dsContent.append(obj);
    }

    // Set the initial sentence of the file
    sentenceWidget->setCurrentRow(fileSwitchDirection ? 0 : (dsContent.size() - 1));
    fileSwitchDirection = true; // Reset the direction state
}

void MainWindow::reloadDsSentenceRequested() {
    if (currentRow >= 0)
        f0Widget->setDsSentenceContent(dsContent[currentRow]);
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
    f0Widget->pullConfig(cfg);
    QString keyConfPath = qApp->applicationDirPath() + "/slurcutter_config.json";
    QFile file(keyConfPath);
    QAS::JsonStream stream;
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
        switchSentence(false);
    } else if (action == nextAction) {
        switchSentence(true);
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
    pullEditedMidi();
    QFileInfo info = fsModel->fileInfo(current);
    if (info.isFile()) {
        if (QMFs::isFileExist(lastFile)) {
            if (!saveFile(lastFile)) {
                return;
            }
        }
        lastFile = info.absoluteFilePath();
        openFile(lastFile);
    }
}


void MainWindow::_q_sentenceChanged(int currentRow) {
    if (currentRow < 0)
        return;
    pullEditedMidi();
    this->currentRow = currentRow;
    f0Widget->setDsSentenceContent(dsContent[currentRow]);
    auto item = sentenceWidget->item(currentRow);
    double offset = item->data(Qt::UserRole + 1).toDouble(), dur = item->data(Qt::UserRole + 2).toDouble();
    playerWidget->setRange(offset, offset + dur);
    f0Widget->setPlayheadPos(0.0);
}
