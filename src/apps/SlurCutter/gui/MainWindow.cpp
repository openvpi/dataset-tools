#include "MainWindow.h"

#include "../SlurCutterKeys.h"

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

#include <QJsonArray>
#include <QStyledItemDelegate>

#include <dstools/ShortcutEditorWidget.h>
#include <dstools/Theme.h>

namespace SlurCutter {
    CustomDelegate::CustomDelegate(const QSet<QString> &editedFiles, QObject *parent)
        : QStyledItemDelegate(parent), m_editedFiles(editedFiles) {
    }

    void CustomDelegate::setEditedFiles(const QSet<QString> &editedFiles) {
        m_editedFiles = editedFiles;
    }

    void CustomDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
        const auto *model = dynamic_cast<const QFileSystemModel *>(index.model());
        if (!model)
            return;

        QStyleOptionViewItem modifiedOption(option);
        const QString fileName = QFileInfo(model->filePath(index)).fileName();

        if (m_editedFiles.contains(fileName))
            modifiedOption.palette.setColor(QPalette::Text, Qt::gray);

        QStyledItemDelegate::paint(painter, modifiedOption, index);
    }

    static QString audioFileToDsFile(const QString &filename) {
        const QFileInfo info(filename);
        const QString suffix = info.suffix().toLower();
        const QString name = info.fileName();
        return info.absolutePath() + "/" + name.mid(0, name.size() - suffix.size() - 1) +
               (suffix != "wav" ? "_" + suffix : "") + ".ds";
    }

    MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_settings("SlurCutter") {
        notifyTimerId = 0;
        playing = false;

        setAcceptDrops(true);

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

        editMenu->addSeparator();
        {
            auto *shortcutAction = new QAction("Shortcut Settings...", this);
            editMenu->addAction(shortcutAction);
            connect(shortcutAction, &QAction::triggered, this, [this]() {
                using dstools::widgets::ShortcutEntry;
                std::vector<ShortcutEntry> entries = {
                    {QStringLiteral("Open Folder"), QStringLiteral("File"), SlurCutterKeys::ShortcutOpen.path, SlurCutterKeys::ShortcutOpen.defaultValue},
                    {QStringLiteral("Previous File"), QStringLiteral("Navigation"), SlurCutterKeys::NavigationPrev.path, SlurCutterKeys::NavigationPrev.defaultValue},
                    {QStringLiteral("Next File"), QStringLiteral("Navigation"), SlurCutterKeys::NavigationNext.path, SlurCutterKeys::NavigationNext.defaultValue},
                    {QStringLiteral("Play/Stop"), QStringLiteral("Playback"), SlurCutterKeys::PlaybackPlay.path, SlurCutterKeys::PlaybackPlay.defaultValue},
                };
                dstools::widgets::ShortcutEditorWidget::showDialog(&m_settings, entries, this);
                applyConfig();
            });
        }

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

        auto *viewMenu = new QMenu("View(&V)", this);
        dstools::Theme::instance().populateThemeMenu(viewMenu);
        bar->addMenu(viewMenu);

        bar->addMenu(helpMenu);

        // Init widgets
        playerWidget = new dstools::widgets::PlayWidget();
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

        m_delegate = new CustomDelegate(editedFiles, treeView);
        treeView->setItemDelegate(m_delegate);

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

        connect(treeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
                &MainWindow::_q_treeCurrentChanged);
        connect(sentenceWidget, &QListWidget::currentRowChanged, this, &MainWindow::_q_sentenceChanged);
        connect(f0Widget, &F0Widget::requestReloadSentence, this, &MainWindow::reloadDsSentenceRequested);

        connect(playerWidget, &dstools::widgets::PlayWidget::playheadChanged, f0Widget, &F0Widget::setPlayHeadPos);

        reloadWindowTitle();
        resize(1280, 720);

        applyConfig();
    }

    MainWindow::~MainWindow() {
        pullEditedMidi();
        if (QFile::exists(lastFile)) {
            saveFile(lastFile);
        }
    }

    void MainWindow::applyConfig() {
        browseAction->setShortcut(m_settings.shortcut(SlurCutterKeys::ShortcutOpen));
        prevAction->setShortcut(m_settings.shortcut(SlurCutterKeys::NavigationPrev));
        nextAction->setShortcut(m_settings.shortcut(SlurCutterKeys::NavigationNext));
        playAction->setShortcut(m_settings.shortcut(SlurCutterKeys::PlaybackPlay));

        f0Widget->loadConfig(m_settings);

        if (const QString savedDir = m_settings.get(SlurCutterKeys::LastDir);
            !savedDir.isEmpty() && QDir(savedDir).exists()) {
            dirname = savedDir;
            openDirectory(dirname);
        }

        reloadWindowTitle();
    }

    void MainWindow::openDirectory(const QString &dirname) {
        this->dirname = dirname;
        editedFiles.clear();

        QFile editFile(dirname + "/SlurEditedFiles.txt");
        if (editFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&editFile);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (!line.isEmpty()) {
                    editedFiles.insert(line);
                }
            }
            editFile.close();
        } else if (!QFile::exists(editFile.fileName())) {
            if (editFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                editFile.close();
            }
        }
        m_delegate->setEditedFiles(editedFiles);
        fsModel->setRootPath(dirname);
        treeView->setRootIndex(fsModel->index(dirname));
    }

    void MainWindow::openFile(const QString &filename) {

        f0Widget->clear();
        sentenceWidget->clear();
        dsContent.clear();

        playerWidget->openFile(filename);

        const QString labFile = audioFileToDsFile(filename);
        QFile file(labFile);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString content = QString::fromUtf8(file.readAll());
            loadDsContent(content);
        } else {
            f0Widget->setErrorStatusText("No DS file can be opened");
        }

        // f0Widget->contentText->setPlainText(content);
    }

    bool MainWindow::saveFile(const QString &filename) {
        QJsonArray docArr;
        for (const auto &i : dsContent) {
            docArr.append(i);
        }
        const QJsonDocument doc(docArr);
        const auto labFile = audioFileToDsFile(filename);
        QFile file(labFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(doc.toJson(QJsonDocument::Indented));
            const QFileInfo info(filename);
            const QString fileName = info.fileName();
            if (!editedFiles.contains(fileName)) {
                editedFiles.insert(fileName);

                QFile editFile(dirname + "/SlurEditedFiles.txt");
                if (editFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
                    QTextStream out(&editFile);
                    out << fileName << "\n";
                    editFile.close();
                }
            }
            m_delegate->setEditedFiles(editedFiles);
            return true;
        }
        QMessageBox::critical(this, "Save error",
                              QString("Cannot open %1 to write!\n\n"
                                      "File switch was not performed. If you were to address the save problem,\n"
                                      "later you can switch file again and saving will be attempted again.")
                                  .arg(file.fileName()));
        return false;
    }

    void MainWindow::pullEditedMidi() {
        if (currentRow < 0 || f0Widget->empty())
            return;
        auto &currentSentence = dsContent[currentRow];
        const auto [note_seq, note_dur, note_slur, note_glide] = f0Widget->getSavedDsStrings();

        currentSentence["note_seq"] = note_seq;
        currentSentence["note_slur"] = note_slur;
        currentSentence["note_dur"] = note_dur;
        currentSentence["note_glide"] = note_glide;
    }

    void MainWindow::switchFile(const bool next) {
        fileSwitchDirection = next;
        if (next) {
            QKeyEvent e(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
            QApplication::sendEvent(treeView, &e);
        } else {
            QKeyEvent e(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
            QApplication::sendEvent(treeView, &e);
        }
    }

    void MainWindow::switchSentence(const bool next) {
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
        const QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), &err);

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
            auto noteDur = noteDuration.split(' ', Qt::SkipEmptyParts);
            const double offset = obj.value("offset").toDouble(-1);
            if (offset == -1) {
                continue;
            }

            double dur = 0.0;
            for (const QString &durStr : noteDur) {
                dur += durStr.toDouble();
            }

            const auto item =
                new QListWidgetItem(QString("%1-%2").arg(offset, 0, 'f', 2, QChar('0')).arg(text.replace(' ', "")));
            item->setData(Qt::UserRole + 1, offset);
            item->setData(Qt::UserRole + 2, dur);
            sentenceWidget->addItem(item);

            dsContent.append(obj);
        }

        // Set the initial sentence of the file
        sentenceWidget->setCurrentRow(fileSwitchDirection ? 0 : dsContent.size() - 1);
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
                                                    QDir::toNativeSeparators(QFileInfo(dirname).baseName())));
    }

    void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
        const auto e = event;
        const QMimeData *mime = e->mimeData();
        if (mime->hasUrls()) {
            auto urls = mime->urls();
            QStringList filenames;
            for (auto it = urls.begin(); it != urls.end(); ++it) {
                if (it->isLocalFile()) {
                    filenames.append(it->toLocalFile());
                }
            }
            bool ok = false;
            if (filenames.size() == 1) {
                const QString filename = filenames.front();
                if (QFile::exists(filename)) {
                    ok = true;
                }
            }
            if (ok) {
                e->acceptProposedAction();
            }
        }
    }

    void MainWindow::dropEvent(QDropEvent *event) {
        const auto e = event;
        const QMimeData *mime = e->mimeData();
        if (mime->hasUrls()) {
            auto urls = mime->urls();
            QStringList filenames;
            for (auto it = urls.begin(); it != urls.end(); ++it) {
                if (it->isLocalFile()) {
                    filenames.append(it->toLocalFile());
                }
            }
            bool ok = false;
            if (filenames.size() == 1) {
                const QString filename = filenames.front();
                if (QFile::exists(filename)) {
                    ok = true;
                    openDirectory(filename);
                    m_settings.set(SlurCutterKeys::LastDir, dirname);
                }
            }
            if (ok) {
                e->acceptProposedAction();
            }
        }
    }

    void MainWindow::closeEvent(QCloseEvent *event) {
        // Pull and save config
        f0Widget->pullConfig(m_settings);

        m_settings.setShortcut(SlurCutterKeys::ShortcutOpen, browseAction->shortcut());
        m_settings.setShortcut(SlurCutterKeys::NavigationPrev, prevAction->shortcut());
        m_settings.setShortcut(SlurCutterKeys::NavigationNext, nextAction->shortcut());
        m_settings.setShortcut(SlurCutterKeys::PlaybackPlay, playAction->shortcut());

        if (!dirname.isEmpty()) {
            m_settings.set(SlurCutterKeys::LastDir, dirname);
        }

        event->accept();
    }

    void MainWindow::_q_fileMenuTriggered(const QAction *action) {
        if (action == browseAction) {
            playerWidget->setPlaying(false);

            const QString path =
                QFileDialog::getExistingDirectory(this, "Open Folder", QFileInfo(dirname).absolutePath());
            if (path.isEmpty()) {
                return;
            }
            openDirectory(path);

            dirname = path;
            m_settings.set(SlurCutterKeys::LastDir, dirname);
        }
        reloadWindowTitle();
    }

    void MainWindow::_q_editMenuTriggered(const QAction *action) {
        if (action == prevAction) {
            switchSentence(false);
        } else if (action == nextAction) {
            switchSentence(true);
        }
    }

    void MainWindow::_q_playMenuTriggered(const QAction *action) const {
        if (action == playAction) {
            playerWidget->setPlaying(!playerWidget->isPlaying());
        }
    }

    void MainWindow::_q_helpMenuTriggered(const QAction *action) {
        if (action == aboutAppAction) {
            QMessageBox::information(this, qApp->applicationName(),
                                     QString("%1 %2, Copyright OpenVPI.").arg(qApp->applicationName(), APP_VERSION));
        } else if (action == aboutQtAction) {
            QMessageBox::aboutQt(this);
        }
    }

    void MainWindow::_q_treeCurrentChanged(const QModelIndex &current) {
        pullEditedMidi();
        const QFileInfo info = fsModel->fileInfo(current);
        if (info.isFile()) {
            if (QFile::exists(lastFile)) {
                if (!saveFile(lastFile)) {
                    return;
                }
            }
            lastFile = info.absoluteFilePath();
            openFile(lastFile);
        }
    }


    void MainWindow::_q_sentenceChanged(const int currentRow) {
        if (currentRow < 0)
            return;
        pullEditedMidi();
        this->currentRow = currentRow;
        f0Widget->setDsSentenceContent(dsContent[currentRow]);
        const auto item = sentenceWidget->item(currentRow);
        const double offset = item->data(Qt::UserRole + 1).toDouble();
        const double dur = item->data(Qt::UserRole + 2).toDouble();
        playerWidget->setPlayRange(offset, offset + dur);
        f0Widget->setPlayHeadPos(0.0);
    }
}