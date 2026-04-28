#include "MainWindow.h"

#include "ExportDialog.h"
#include "../MinLabelKeys.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QStatusBar>
#include <QTreeWidget>

#include <dstools/ShortcutManager.h>
#include <dstools/Theme.h>

namespace Minlabel {
    MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_settings("MinLabel") {
        playing = false;

        setAcceptDrops(true);

        // Init menus
        browseAction = new QAction("Open Folder", this);

        convertAction = new QAction("Convert lab to project file", this);

        exportAction = new QAction("Export", this);

        fileMenu = new QMenu("File(&F)", this);
        fileMenu->addAction(browseAction);
        fileMenu->addAction(exportAction);
        fileMenu->addAction(convertAction);

        nextAction = new QAction("Next file", this);

        prevAction = new QAction("Previous file", this);

        editMenu = new QMenu("Edit(&E)", this);
        editMenu->addAction(nextAction);
        editMenu->addAction(prevAction);

        editMenu->addSeparator();
        {
            auto *shortcutAction = new QAction("Shortcut Settings...", this);
            editMenu->addAction(shortcutAction);
            connect(shortcutAction, &QAction::triggered, this, [this]() {
                m_shortcutManager->showEditor(this);
            });
        }

        playAction = new QAction("Play/Stop", this);

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

        auto *viewMenu = new QMenu("View(&V)", this);
        dstools::Theme::instance().populateThemeMenu(viewMenu);
        bar->addMenu(viewMenu);

        bar->addMenu(helpMenu);

        m_shortcutManager = new dstools::widgets::ShortcutManager(&m_settings, this);
        m_shortcutManager->bind(browseAction, MinLabelKeys::ShortcutOpen, QStringLiteral("Open Folder"), QStringLiteral("File"));
        m_shortcutManager->bind(exportAction, MinLabelKeys::ShortcutExport, QStringLiteral("Export"), QStringLiteral("File"));
        m_shortcutManager->bind(prevAction, MinLabelKeys::NavigationPrev, QStringLiteral("Previous File"), QStringLiteral("Navigation"));
        m_shortcutManager->bind(nextAction, MinLabelKeys::NavigationNext, QStringLiteral("Next File"), QStringLiteral("Navigation"));
        m_shortcutManager->bind(playAction, MinLabelKeys::PlaybackPlay, QStringLiteral("Play/Stop"), QStringLiteral("Playback"));
        m_shortcutManager->applyAll();

        m_fileProgress = new dstools::widgets::FileProgressTracker(
            dstools::widgets::FileProgressTracker::ProgressBarStyle, this);

        // Init widgets
        playerWidget = new dstools::widgets::PlayWidget();
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

        auto *delegate = new dstools::widgets::FileStatusDelegate(treeView);
        delegate->setStatusChecker([](const QString &filePath) -> bool {
            const QFileInfo fileInfo(filePath);
            const QString jsonFilePath = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName() + ".json";
            if (!QFile::exists(jsonFilePath) || !fileInfo.isFile())
                return false;
            QFile jsonFile(jsonFilePath);
            if (!jsonFile.open(QIODevice::ReadOnly))
                return false;
            const QByteArray jsonData = jsonFile.readAll();
            jsonFile.close();
            const QJsonDocument jsonDoc(QJsonDocument::fromJson(jsonData));
            if (!jsonDoc.isObject())
                return false;
            return jsonDoc.object().value("isCheck").toBool(false);
        });
        treeView->setItemDelegate(delegate);

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
        rightLayout->addWidget(m_fileProgress);

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

        connect(treeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
                &MainWindow::_q_treeCurrentChanged);
        connect(treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                &MainWindow::_q_updateProgress);

        reloadWindowTitle();
        resize(1280, 720);

        applyConfig();
    }

    MainWindow::~MainWindow() {
        if (QFile::exists(lastFile)) {
            saveFile(lastFile);
        }
    }

    void MainWindow::openDirectory(const QString &dirName) const {
        fsModel->setRootPath(dirName);
        treeView->setRootIndex(fsModel->index(dirName));
    }

    void MainWindow::openFile(const QString &filename) const {
        QString labContent, txtContent;

        const QString jsonFilePath = audioToOtherSuffix(filename, "json");
        if (QJsonObject readData; readJsonFile(jsonFilePath, readData)) {
            txtContent = readData.contains("raw_text") ? readData["raw_text"].toString() : "";
            labContent = readData.contains("lab") ? readData["lab"].toString() : "";
        }
        textWidget->contentText->setPlainText(labContent);
        textWidget->wordsText->setText(txtContent);

        playerWidget->openFile(filename);
    }

    bool MainWindow::saveFile(const QString &filename) {
        const QString txtContent = textWidget->wordsText->text();
        QString labContent = textWidget->contentText->toPlainText();
        QString withoutTone = "";
        static QRegularExpression rm_alpha("[^a-z]");
        if (!labContent.isEmpty() && labContent.contains(" ")) {
            QStringList inputList = labContent.split(" ");
            for (QString &item : inputList) {
                item.remove(rm_alpha);
            }
            withoutTone = inputList.join(" ");
        }


        const QString jsonFilePath = audioToOtherSuffix(filename, "json");

        if (labContent.isEmpty() && txtContent.isEmpty() && !QFile::exists(jsonFilePath)) {
            return true;
        }

        static QRegularExpression rm_s("\\s+");

        QFile jsonFile(jsonFilePath);
        QJsonObject writeData;
        writeData["lab"] = labContent.replace(rm_s, " ");
        writeData["raw_text"] = txtContent;
        writeData["lab_without_tone"] = withoutTone.replace(rm_s, " ");
        writeData["isCheck"] = true;

        if (!writeJsonFile(jsonFilePath, writeData)) {
            QMessageBox::critical(this, QApplication::applicationName(),
                                  QString("Failed to write to file %1").arg(jsonFilePath));
            return false;
        }

        const QString labFilePath = audioToOtherSuffix(filename, "lab");

        if (labContent.isEmpty() && !QFile::exists(labFilePath)) {
            return true;
        }

        QFile labFile(labFilePath);
        if (!labFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::critical(this, QApplication::applicationName(),
                                  QString("Failed to write to file %1").arg(labFilePath));
            return false;
        }

        QTextStream labIn(&labFile);
        labIn << labContent;
        labFile.close();
        return true;
    }

    void MainWindow::reloadWindowTitle() {
        setWindowTitle(dirname.isEmpty()
                           ? QApplication::applicationName()
                           : QString("%1 - %2").arg(QApplication::applicationName(),
                                                    QDir::toNativeSeparators(QFileInfo(dirname).baseName())));
    }

    void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
        const QMimeData *mime = event->mimeData();
        if (mime->hasUrls()) {
            auto urls = mime->urls();
            QStringList filenames;
            for (auto &url : urls) {
                if (url.isLocalFile()) {
                    filenames.append(url.toLocalFile());
                }
            }
            bool ok = false;
            if (filenames.size() == 1) {
                if (const QString &filename = filenames.front(); QFile::exists(filename)) {
                    ok = true;
                }
            }
            if (ok) {
                event->acceptProposedAction();
            }
        }
    }

    void MainWindow::dropEvent(QDropEvent *event) {
        if (const QMimeData *mime = event->mimeData(); mime->hasUrls()) {
            auto urls = mime->urls();
            QStringList filenames;
            for (auto &url : urls) {
                if (url.isLocalFile()) {
                    filenames.append(url.toLocalFile());
                }
            }
            bool ok = false;
            if (filenames.size() == 1) {
                if (const QString &filename = filenames.front(); QFile::exists(filename)) {
                    ok = true;
                    openDirectory(filename);

                    dirname = filename;
                    m_settings.set(MinLabelKeys::LastDir, dirname);
                    _q_updateProgress();
                }
            }
            if (ok) {
                event->acceptProposedAction();
            }
        }
    }

    void MainWindow::closeEvent(QCloseEvent *event) {
        m_shortcutManager->saveAll();

        if (!dirname.isEmpty()) {
            m_settings.set(MinLabelKeys::LastDir, dirname);
        }

        event->accept();
    }

    void MainWindow::applyConfig() {
        m_shortcutManager->applyAll();

        if (const QString savedDir = m_settings.get(MinLabelKeys::LastDir);
            !savedDir.isEmpty() && QDir(savedDir).exists()) {
            dirname = savedDir;
            openDirectory(dirname);
            _q_updateProgress();
        }

        reloadWindowTitle();
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
            m_settings.set(MinLabelKeys::LastDir, dirname);
            _q_updateProgress();
        } else if (action == convertAction) {
            playerWidget->setPlaying(false);
            const int choice = QMessageBox::question(this, "Convert lab to project file.",
                                                     "Do you want to convert lab to project file in target folder?",
                                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (choice == QMessageBox::Yes) {
                const QString path =
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

    void MainWindow::_q_helpMenuTriggered(const QAction *action) {
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
        const int total = static_cast<int>(fsModel->rootDirectory().entryList(
            QStringList{"*.wav", "*.mp3", "*.m4a", "*.flac"}, QDir::Files).count());
        m_fileProgress->setProgress(count, total);
    }

    void MainWindow::_q_treeCurrentChanged(const QModelIndex &current) {
        const QFileInfo info = fsModel->fileInfo(current);
        if (info.isFile()) {
            if (QFile::exists(lastFile)) {
                saveFile(lastFile);
            }
            lastFile = info.absoluteFilePath();
            textWidget->wordsText->clear();
            openFile(lastFile);
        }
    }

    auto MainWindow::exportAudio(const ExportInfo &exportInfo) -> void {
        const int count = jsonCount(dirname);
        if (const int totalRowCount = static_cast<int>(fsModel->rootDirectory().entryList(
                QStringList{"*.wav", "*.mp3", "*.m4a", "*.flac"}, QDir::Files).count()); totalRowCount != count) {
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
        for (const QFileInfo &fileInfo : fileInfoList) {
            QString currentFilePath = fsModel->fileInfo(treeView->currentIndex()).absoluteFilePath();
            QString labFilePath = fileInfo.absoluteFilePath();
            QString suffix = fileInfo.suffix().toLower();
            QString name = fileInfo.fileName();
            if (QString jsonFilePath =
                    fileInfo.absolutePath() + "/" + name.mid(0, name.size() - suffix.size() - 1) + ".json";
                fileInfo.suffix() == "lab" && !QFile::exists(jsonFilePath)) {
                if (QFile file(labFilePath); file.open(QIODevice::ReadOnly | QIODevice::Text)) {
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
                                item.remove(QRegularExpression("[^a-z]"));
                            }
                            withoutTone = inputList.join(" ");
                        }

                        QFile jsonFile(jsonFilePath);
                        QJsonObject writeData;
                        writeData["lab"] = labContent.replace(QRegularExpression("\\s+"), " ");
                        writeData["raw_text"] = txtContent;
                        writeData["lab_without_tone"] = withoutTone.replace(QRegularExpression("\\s+"), " ");

                        if (!writeJsonFile(jsonFilePath, writeData)) {
                            QMessageBox::critical(this, QApplication::applicationName(),
                                                  QString("Failed to write to file %1").arg(jsonFilePath));
                            continue;
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
}