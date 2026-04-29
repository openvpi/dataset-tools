#include "MinLabelPage.h"

#include "ExportDialog.h"
#include "../MinLabelKeys.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QHeaderView>
#include <QKeyEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QTreeWidget>

#include <dstools/ShortcutManager.h>

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

        applyConfig();
    }

    MinLabelPage::~MinLabelPage() {
        if (QFile::exists(lastFile)) {
            saveFile(lastFile);
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
        return QFile::exists(lastFile);
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

    void MinLabelPage::openDirectory(const QString &dirName) const {
        fsModel->setRootPath(dirName);
        treeView->setRootIndex(fsModel->index(dirName));
    }

    void MinLabelPage::openFile(const QString &filename) const {
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

    bool MinLabelPage::saveFile(const QString &filename) {
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
        // This is now handled by MainWindow directly
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

    void MinLabelPage::labToJson(const QString &dirName) {
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

} // namespace Minlabel
