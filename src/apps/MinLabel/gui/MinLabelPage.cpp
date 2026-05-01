#include "MinLabelPage.h"

#include "ExportDialog.h"
#include "../MinLabelKeys.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QTreeWidget>

#include <MinLabelService.h>
#include <dstools/AudioFileResolver.h>
#include <dstools/ShortcutManager.h>
#include <dsfw/Theme.h>

namespace Minlabel {

    MinLabelPage::MinLabelPage(QWidget *parent) : QWidget(parent), m_settings("MinLabel") {
        playing = false;

        // Init actions
        m_browseAction = new QAction("Open Folder", this);

        m_convertAction = new QAction("Convert lab to project file", this);

        m_exportAction = new QAction("Export", this);

        m_nextAction = new QAction("Next file", this);

        m_prevAction = new QAction("Previous file", this);

        m_playAction = new QAction("Play/Stop", this);

        m_shortcutManager = new dstools::widgets::ShortcutManager(&m_settings, this);
        m_shortcutManager->bind(m_browseAction, MinLabelKeys::ShortcutOpen, QStringLiteral("Open Folder"), QStringLiteral("File"));
        m_shortcutManager->bind(m_exportAction, MinLabelKeys::ShortcutExport, QStringLiteral("Export"), QStringLiteral("File"));
        m_shortcutManager->bind(m_prevAction, MinLabelKeys::NavigationPrev, QStringLiteral("Previous File"), QStringLiteral("Navigation"));
        m_shortcutManager->bind(m_nextAction, MinLabelKeys::NavigationNext, QStringLiteral("Next File"), QStringLiteral("Navigation"));
        m_shortcutManager->bind(m_playAction, MinLabelKeys::PlaybackPlay, QStringLiteral("Play/Stop"), QStringLiteral("Playback"));
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
            auto result = MinLabelService::loadLabel(
                dstools::AudioFileResolver::audioToDataFile(filePath, "json"));
            return result && result.value().isCheck;
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

        // Set mainSplitter as the layout of this widget
        auto *pageLayout = new QVBoxLayout(this);
        pageLayout->setContentsMargins(0, 0, 0, 0);
        pageLayout->addWidget(mainSplitter);

        mainSplitter->setStretchFactor(0, 0);
        mainSplitter->setStretchFactor(1, 1);

        // Init signals
        connect(m_browseAction, &QAction::triggered, this, [this]() {
            _q_fileMenuTriggered(m_browseAction);
        });
        connect(m_convertAction, &QAction::triggered, this, [this]() {
            _q_fileMenuTriggered(m_convertAction);
        });
        connect(m_exportAction, &QAction::triggered, this, [this]() {
            _q_fileMenuTriggered(m_exportAction);
        });
        connect(m_nextAction, &QAction::triggered, this, [this]() {
            _q_editMenuTriggered(m_nextAction);
        });
        connect(m_prevAction, &QAction::triggered, this, [this]() {
            _q_editMenuTriggered(m_prevAction);
        });
        connect(m_playAction, &QAction::triggered, this, [this]() {
            _q_playMenuTriggered(m_playAction);
        });

        connect(treeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
                &MinLabelPage::_q_treeCurrentChanged);
        connect(treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this,
                &MinLabelPage::_q_updateProgress);

        connect(textWidget->contentText, &QPlainTextEdit::textChanged, this, [this]() {
            m_isDirty = true;
        });
        connect(textWidget->wordsText, &QLineEdit::textChanged, this, [this]() {
            m_isDirty = true;
        });

        applyConfig();
    }

    MinLabelPage::~MinLabelPage() {
        onShutdown();

        if (QFile::exists(lastFile)) {
            saveFile(lastFile);
        }
    }

    bool MinLabelPage::onDeactivating() {
        if (hasUnsavedChanges()) {
            const auto result = QMessageBox::question(
                this, tr("Unsaved Changes"),
                tr("You have unsaved changes. Do you want to save before closing?"),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                QMessageBox::Save);
            if (result == QMessageBox::Cancel)
                return false;
            if (result == QMessageBox::Save)
                save();
        }
        return true;
    }

    void MinLabelPage::onShutdown() {
        m_shortcutManager->saveAll();

        if (!dirname.isEmpty()) {
            m_settings.set(MinLabelKeys::LastDir, dirname);
        }
    }

    QAction *MinLabelPage::browseAction() const {
        return m_browseAction;
    }

    QAction *MinLabelPage::convertAction() const {
        return m_convertAction;
    }

    QAction *MinLabelPage::exportAction() const {
        return m_exportAction;
    }

    QAction *MinLabelPage::nextAction() const {
        return m_nextAction;
    }

    QAction *MinLabelPage::prevAction() const {
        return m_prevAction;
    }

    QAction *MinLabelPage::playAction() const {
        return m_playAction;
    }

    void MinLabelPage::setWorkingDirectory(const QString &dir) {
        if (dir.isEmpty() || !QDir(dir).exists())
            return;
        openDirectory(dir);
        dirname = dir;
        m_settings.set(MinLabelKeys::LastDir, dirname);
        _q_updateProgress();
        emit workingDirectoryChanged(dirname);
    }

    QString MinLabelPage::workingDirectory() const {
        return dirname;
    }

    bool MinLabelPage::hasUnsavedChanges() const {
        return m_isDirty;
    }

    bool MinLabelPage::save() {
        if (QFile::exists(lastFile)) {
            return saveFile(lastFile);
        }
        return true;
    }

    dstools::AppSettings &MinLabelPage::settings() {
        return m_settings;
    }

    dstools::widgets::ShortcutManager *MinLabelPage::shortcutManager() const {
        return m_shortcutManager;
    }

    QMenuBar *MinLabelPage::createMenuBar(QWidget *parent) {
        auto *bar = new QMenuBar(parent);

        auto *fileMenu = bar->addMenu(tr("File(&F)"));
        fileMenu->addAction(m_browseAction);
        fileMenu->addAction(m_exportAction);
        fileMenu->addAction(m_convertAction);

        auto *editMenu = bar->addMenu(tr("Edit(&E)"));
        editMenu->addAction(m_nextAction);
        editMenu->addAction(m_prevAction);
        editMenu->addSeparator();
        {
            auto *shortcutAction = editMenu->addAction(tr("Shortcut Settings..."));
            connect(shortcutAction, &QAction::triggered, this, [this]() {
                m_shortcutManager->showEditor(this);
            });
        }

        auto *playMenu = bar->addMenu(tr("Playback(&P)"));
        playMenu->addAction(m_playAction);

        auto *viewMenu = bar->addMenu(tr("View(&V)"));
        dsfw::Theme::instance().populateThemeMenu(viewMenu);

        auto *helpMenu = bar->addMenu(tr("Help(&H)"));
        auto *aboutAppAction = helpMenu->addAction(
            tr("About %1").arg(QApplication::applicationName()));
        auto *aboutQtAction = helpMenu->addAction(tr("About Qt"));

        connect(helpMenu, &QMenu::triggered, this, [this](const QAction *action) {
            if (action->text().startsWith("About Qt")) {
                QMessageBox::aboutQt(this);
            } else {
                QMessageBox::information(
                    this, QApplication::applicationName(),
                    QString("%1 %2, Copyright OpenVPI.")
                        .arg(QApplication::applicationName(), QApplication::applicationVersion()));
            }
        });

        return bar;
    }

    QWidget *MinLabelPage::createStatusBarContent(QWidget *parent) {
        Q_UNUSED(parent);
        return nullptr;
    }

    QString MinLabelPage::windowTitle() const {
        return dirname.isEmpty()
                   ? QApplication::applicationName()
                   : QString("%1 - %2").arg(QApplication::applicationName(),
                                            QDir::toNativeSeparators(QFileInfo(dirname).baseName()));
    }

    bool MinLabelPage::supportsDragDrop() const {
        return true;
    }

    void MinLabelPage::handleDragEnter(QDragEnterEvent *event) {
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

    void MinLabelPage::handleDrop(QDropEvent *event) {
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
                    const QFileInfo fi(filename);
                    setWorkingDirectory(fi.isDir() ? filename : fi.absolutePath());
                }
            }
            if (ok) {
                event->acceptProposedAction();
            }
        }
    }

    void MinLabelPage::openDirectory(const QString &dirName) const {
        fsModel->setRootPath(dirName);
        treeView->setRootIndex(fsModel->index(dirName));
    }

    void MinLabelPage::openFile(const QString &filename) const {
        const QString jsonFilePath = audioToOtherSuffix(filename, "json");
        auto result = MinLabelService::loadLabel(jsonFilePath);
        if (result) {
            textWidget->contentText->setPlainText(result.value().lab);
            textWidget->wordsText->setText(result.value().rawText);
        } else {
            textWidget->contentText->clear();
            textWidget->wordsText->clear();
        }

        playerWidget->openFile(filename);
    }

    bool MinLabelPage::saveFile(const QString &filename) {
        const QString txtContent = textWidget->wordsText->text();
        const QString labContent = textWidget->contentText->toPlainText();

        const QString jsonFilePath = audioToOtherSuffix(filename, "json");

        if (labContent.isEmpty() && txtContent.isEmpty() && !QFile::exists(jsonFilePath)) {
            return true;
        }

        LabelData data;
        data.lab = labContent;
        data.rawText = txtContent;
        data.isCheck = true;

        const QString labFilePath = audioToOtherSuffix(filename, "lab");
        auto result = MinLabelService::saveLabel(jsonFilePath, labFilePath, data);
        if (!result) {
            QMessageBox::critical(this, QApplication::applicationName(),
                                  QString("Failed to write to file: %1").arg(QString::fromStdString(result.error())));
            return false;
        }

        m_isDirty = false;
        return true;
    }

    void MinLabelPage::applyConfig() {
        m_shortcutManager->applyAll();

        if (const QString savedDir = m_settings.get(MinLabelKeys::LastDir);
            !savedDir.isEmpty() && QDir(savedDir).exists()) {
            dirname = savedDir;
            openDirectory(dirname);
            _q_updateProgress();
        }

        emit workingDirectoryChanged(dirname);
    }

    void MinLabelPage::_q_fileMenuTriggered(const QAction *action) {
        if (action == m_browseAction) {
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
        } else if (action == m_convertAction) {
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
        } else if (action == m_exportAction) {
            playerWidget->setPlaying(false);
            ExportDialog dialog(this);

            if (dialog.exec() == QDialog::Accepted) {
                exportAudio(dialog.exportInfo);
            }
        }
        emit workingDirectoryChanged(dirname);
    }

    void MinLabelPage::_q_editMenuTriggered(const QAction *action) const {
        if (action == m_prevAction) {
            QKeyEvent e(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
            QApplication::sendEvent(treeView, &e);
        } else if (action == m_nextAction) {
            QKeyEvent e(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
            QApplication::sendEvent(treeView, &e);
        }
    }

    void MinLabelPage::_q_playMenuTriggered(const QAction *action) const {
        if (action == m_playAction) {
            playerWidget->setPlaying(!playerWidget->isPlaying());
        }
    }

    void MinLabelPage::_q_helpMenuTriggered(const QAction *action) {
        Q_UNUSED(action)
    }

    void MinLabelPage::_q_updateProgress() const {
        const int count = jsonCount(dirname);
        const int total = static_cast<int>(fsModel->rootDirectory().entryList(
            QStringList{"*.wav", "*.mp3", "*.m4a", "*.flac"}, QDir::Files).count());
        m_fileProgress->setProgress(count, total);
    }

    void MinLabelPage::_q_treeCurrentChanged(const QModelIndex &current) {
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

    auto MinLabelPage::exportAudio(const ExportInfo &exportInfo) -> void {
        const int count = jsonCount(dirname);
        if (const int totalRowCount = static_cast<int>(fsModel->rootDirectory().entryList(
                QStringList{"*.wav", "*.mp3", "*.m4a", "*.flac"}, QDir::Files).count()); totalRowCount != count) {
            const QMessageBox::StandardButton reply =
                QMessageBox::question(this, QApplication::applicationName(),
                                      QString("%1 files are not labeled, continue?").arg(totalRowCount - count),
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

    void MinLabelPage::labToJson(const QString &dirName) {
        auto result = MinLabelService::convertLabToJson(dirName);
        if (!result) {
            QMessageBox::critical(this, QApplication::applicationName(),
                                  QString("Conversion failed: %1").arg(QString::fromStdString(result.error())));
            return;
        }

        const auto &convResult = result.value();

        QString currentFilePath = fsModel->fileInfo(treeView->currentIndex()).absoluteFilePath();
        QString currentLabPath = audioToOtherSuffix(currentFilePath, "lab");

        QDir directory(dirName);
        QFileInfoList fileInfoList = directory.entryInfoList(QDir::Files);
        for (const QFileInfo &fileInfo : fileInfoList) {
            if (fileInfo.suffix().toLower() != "lab")
                continue;
            QString labFilePath = fileInfo.absoluteFilePath();
            if (currentLabPath == labFilePath) {
                auto labelResult = MinLabelService::loadLabel(
                    audioToOtherSuffix(currentFilePath, "json"));
                if (labelResult) {
                    textWidget->contentText->setPlainText(labelResult.value().lab);
                    textWidget->wordsText->setText(labelResult.value().rawText);
                }
                break;
            }
        }

        _q_updateProgress();
        QMessageBox::information(this, QApplication::applicationName(),
                                 QString("Convert %1 lab files to current project file.").arg(convResult.count));
    }

} // namespace Minlabel
