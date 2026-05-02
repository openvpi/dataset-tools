#include "PitchLabelerPage.h"

#include "DSFile.h"
#include "PitchFileService.h"
#include "../PitchLabelerKeys.h"
#include "ui/FileListPanel.h"

#include <dstools/TimePos.h>

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QMessageBox>
#include <QShortcut>
#include <QVBoxLayout>

#include <dstools/ShortcutManager.h>
#include <dstools/AudioFileResolver.h>
#include <dsfw/Theme.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>

#include <functional>

namespace dstools {
    namespace pitchlabeler {

        PitchLabelerPage::PitchLabelerPage(QWidget *parent)
            : QWidget(parent),
              m_editor(new PitchEditor(this)),
              m_fileService(new PitchFileService(this))
        {
            // Layout: FileList | Editor
            m_outerSplitter = new QSplitter(Qt::Horizontal);

            m_fileListPanel = new ui::FileListPanel();
            m_fileListPanel->setMinimumWidth(160);
            m_fileListPanel->setMaximumWidth(280);
            m_outerSplitter->addWidget(m_fileListPanel);
            m_outerSplitter->addWidget(m_editor);
            m_outerSplitter->setSizes({200, 1240});

            auto *pageLayout = new QVBoxLayout(this);
            pageLayout->setContentsMargins(0, 0, 0, 0);
            pageLayout->setSpacing(0);
            pageLayout->addWidget(m_outerSplitter);

            m_shortcutManager = new dstools::widgets::ShortcutManager(&m_settings, this);

            // Connect file list
            connect(m_fileListPanel, &ui::FileListPanel::fileSelected, this, &PitchLabelerPage::loadFile);

            // Connect editor signals
            connect(m_editor->saveAction(), &QAction::triggered, this, &PitchLabelerPage::saveFile);
            connect(m_editor->saveAllAction(), &QAction::triggered, this, &PitchLabelerPage::saveAllFiles);

            connect(m_editor, &PitchEditor::fileEdited, this, [this]() {
                if (m_currentFile) {
                    m_fileService->markCurrentFileModified();
                    updateStatusInfo();
                    m_fileListPanel->setFileModified(m_currentFilePath, true);
                    emit modificationChanged(true);
                }
            });

            connect(m_editor, &PitchEditor::modificationChanged, this, &PitchLabelerPage::modificationChanged);

            applyConfig();
        }

        PitchLabelerPage::~PitchLabelerPage() = default;

        void PitchLabelerPage::setWorkingDirectory(const QString &dir) {
            m_workingDirectory = dir;
            m_fileListPanel->setDirectory(dir);
            m_settings.set(PitchLabelerKeys::LastDir, dir);
            emit workingDirectoryChanged(dir);
        }

        void PitchLabelerPage::closeDirectory() {
            if (m_fileService->hasUnsavedChanges()) {
                auto ret = QMessageBox::question(this, QStringLiteral("未保存的更改"),
                                                 QStringLiteral("关闭前是否保存？"),
                                                 QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
                if (ret == QMessageBox::Save) {
                    m_fileService->saveFile();
                } else if (ret == QMessageBox::Cancel) {
                    return;
                }
            }

            m_fileListPanel->saveState();

            m_workingDirectory.clear();
            m_currentFilePath.clear();
            m_currentFile.reset();

            m_editor->clear();
            m_fileListPanel->clear();

            updateStatusInfo();
            emit workingDirectoryChanged(QString());
        }

        bool PitchLabelerPage::hasUnsavedChanges() const {
            return m_fileService->hasUnsavedChanges();
        }

        bool PitchLabelerPage::save() {
            return m_fileService->saveFile();
        }

        bool PitchLabelerPage::saveAll() {
            return m_fileService->saveAllFiles();
        }

        QString PitchLabelerPage::windowTitle() const {
            QString title = QStringLiteral("DiffSinger 音高标注器");
            if (!m_workingDirectory.isEmpty() && !m_currentFilePath.isEmpty()) {
                QString fileName = QFileInfo(m_currentFilePath).fileName();
                title = fileName + (hasUnsavedChanges() ? " *" : "") + " - " + title;
            }
            return title;
        }

        void PitchLabelerPage::openDirectory() {
            QString dir =
                QFileDialog::getExistingDirectory(this, QStringLiteral("打开工作目录"), QString(), QFileDialog::ShowDirsOnly);
            if (dir.isEmpty())
                return;
            setWorkingDirectory(dir);
        }

        void PitchLabelerPage::saveConfig() {
            if (!m_workingDirectory.isEmpty()) {
                m_settings.set(PitchLabelerKeys::LastDir, m_workingDirectory);
            }
            m_editor->pullConfig(m_settings);
            m_fileListPanel->saveState();
        }

        void PitchLabelerPage::applyConfig() {
            m_shortcutManager->applyAll();
            rebuildWindowShortcuts();
            m_editor->loadConfig(m_settings);

            if (const QString savedDir = m_settings.get(PitchLabelerKeys::LastDir);
                !savedDir.isEmpty() && QDir(savedDir).exists()) {
                m_workingDirectory = savedDir;
                m_fileListPanel->setDirectory(savedDir);
            }
        }

        void PitchLabelerPage::loadFile(const QString &path) {
            if (!m_fileService->loadFile(path))
                return;

            m_currentFile = m_fileService->currentFile();
            m_currentFilePath = m_fileService->currentFilePath();

            m_editor->loadDSFile(m_currentFile);

            QString audioPath = dstools::AudioFileResolver::findAudioFile(path);
            if (!audioPath.isEmpty()) {
                m_editor->loadAudio(audioPath, m_editor->playWidget()->duration());
            }

            m_originalF0 = m_currentFile->f0.values;

            updateStatusInfo();
            emit fileLoaded(path);
            emit fileSelected(path);
        }

        bool PitchLabelerPage::saveFile() {
            if (!m_fileService->saveFile())
                return false;

            updateStatusInfo();
            m_fileListPanel->setFileSaved(m_currentFilePath);
            emit fileSaved(m_currentFilePath);
            emit modificationChanged(false);
            return true;
        }

        bool PitchLabelerPage::saveAllFiles() {
            if (m_fileService->hasUnsavedChanges()) {
                return saveFile();
            }
            return true;
        }

        void PitchLabelerPage::onNextFile() {
            m_fileListPanel->selectNextFile();
        }

        void PitchLabelerPage::onPrevFile() {
            m_fileListPanel->selectPrevFile();
        }

        void PitchLabelerPage::updateStatusInfo() {
            if (!m_currentFilePath.isEmpty()) {
                emit fileStatusChanged(QFileInfo(m_currentFilePath).fileName());
            } else {
                emit fileStatusChanged(QString());
            }

            emit modificationChanged(hasUnsavedChanges());
        }

        void PitchLabelerPage::rebuildWindowShortcuts() {
            qDeleteAll(m_windowShortcuts);
            m_windowShortcuts.clear();

            struct Entry {
                const dstools::SettingsKey<QString> &key;
                std::function<void()> action;
            };

            const Entry entries[] = {
                {PitchLabelerKeys::ToolSelect, [this]() { m_editor->pianoRoll()->setToolMode(ui::ToolSelect); }},
                {PitchLabelerKeys::ToolModulation, [this]() { m_editor->pianoRoll()->setToolMode(ui::ToolModulation); }},
                {PitchLabelerKeys::ToolDrift, [this]() { m_editor->pianoRoll()->setToolMode(ui::ToolDrift); }},
                {PitchLabelerKeys::PlaybackPlayPause, [this]() { m_editor->playWidget()->setPlaying(!m_editor->playWidget()->isPlaying()); }},
                {PitchLabelerKeys::PlaybackStop, [this]() { m_editor->playWidget()->setPlaying(false); }},
                {PitchLabelerKeys::NavigationPrev, [this]() { onPrevFile(); }},
                {PitchLabelerKeys::NavigationNext, [this]() { onNextFile(); }},
            };

            for (const auto &[key, action] : entries) {
                const QKeySequence ks = m_settings.shortcut(key);
                if (!ks.isEmpty()) {
                    auto *sc = new QShortcut(ks, this, nullptr, nullptr, Qt::WindowShortcut);
                    connect(sc, &QShortcut::activated, this, action);
                    m_windowShortcuts.append(sc);
                }
            }
        }

        QMenuBar *PitchLabelerPage::createMenuBar(QWidget *parent) {
            auto *bar = new QMenuBar(parent);

            // ---- 文件 ----
            auto *fileMenu = bar->addMenu(QStringLiteral("文件(&F)"));

            auto *actOpenDir = new QAction(QStringLiteral("打开工作目录(&O)..."), bar);
            actOpenDir->setStatusTip(QStringLiteral("打开包含 .ds 文件的目录"));
            fileMenu->addAction(actOpenDir);
            connect(actOpenDir, &QAction::triggered, this, &PitchLabelerPage::openDirectory);

            auto *actCloseDir = new QAction(QStringLiteral("关闭工作目录(&C)"), bar);
            actCloseDir->setStatusTip(QStringLiteral("关闭当前工作目录"));
            fileMenu->addAction(actCloseDir);
            connect(actCloseDir, &QAction::triggered, this, &PitchLabelerPage::closeDirectory);

            fileMenu->addSeparator();
            fileMenu->addAction(m_editor->saveAction());
            fileMenu->addAction(m_editor->saveAllAction());
            fileMenu->addSeparator();

            auto *actExit = new QAction(QStringLiteral("退出(&X)"), bar);
            actExit->setStatusTip(QStringLiteral("退出应用程序"));
            fileMenu->addAction(actExit);
            connect(actExit, &QAction::triggered, this, [this]() {
                if (auto *w = window()) w->close();
            });

            // ---- 编辑 ----
            auto *editMenu = bar->addMenu(QStringLiteral("编辑(&E)"));
            editMenu->addAction(m_editor->undoAction());
            editMenu->addAction(m_editor->redoAction());

            // ---- 视图 ----
            auto *viewMenu = bar->addMenu(QStringLiteral("视图(&V)"));
            viewMenu->addAction(m_editor->zoomInAction());
            viewMenu->addAction(m_editor->zoomOutAction());
            viewMenu->addAction(m_editor->zoomResetAction());
            viewMenu->addSeparator();
            dsfw::Theme::instance().populateThemeMenu(viewMenu);

            // ---- 工具 ----
            auto *toolsMenu = bar->addMenu(QStringLiteral("工具(&T)"));
            toolsMenu->addAction(m_editor->abCompareAction());
            toolsMenu->addSeparator();
            {
                auto *shortcutAction = new QAction(QStringLiteral("快捷键设置(&K)..."), bar);
                toolsMenu->addAction(shortcutAction);
                connect(shortcutAction, &QAction::triggered, this, [this]() {
                    m_shortcutManager->showEditor(this);
                });
            }

            // ---- 帮助 ----
            auto *helpMenu = bar->addMenu(QStringLiteral("帮助(&H)"));
            auto *actAbout = new QAction(QStringLiteral("关于(&A)"), bar);
            actAbout->setStatusTip(QStringLiteral("关于 DiffSinger 音高标注器"));
            helpMenu->addAction(actAbout);
            connect(actAbout, &QAction::triggered, this, []() {
                QMessageBox::about(nullptr, QStringLiteral("关于 DiffSinger 音高标注器"),
                                   QStringLiteral("DiffSinger 音高标注器\n版本 0.1.0\n\n"
                                   "DiffSinger 数据标注工作流的音高标注编辑器。"));
            });

            // Bind shortcuts
            m_shortcutManager->bind(actOpenDir, PitchLabelerKeys::ShortcutOpen, QStringLiteral("Open Directory"), QStringLiteral("File"));
            m_shortcutManager->bind(m_editor->saveAction(), PitchLabelerKeys::ShortcutSave, QStringLiteral("Save"), QStringLiteral("File"));
            m_shortcutManager->bind(m_editor->saveAllAction(), PitchLabelerKeys::ShortcutSaveAll, QStringLiteral("Save All"), QStringLiteral("File"));
            m_shortcutManager->bind(actExit, PitchLabelerKeys::ShortcutExit, QStringLiteral("Exit"), QStringLiteral("File"));
            m_shortcutManager->bind(m_editor->undoAction(), PitchLabelerKeys::ShortcutUndo, QStringLiteral("Undo"), QStringLiteral("Edit"));
            m_shortcutManager->bind(m_editor->redoAction(), PitchLabelerKeys::ShortcutRedo, QStringLiteral("Redo"), QStringLiteral("Edit"));
            m_shortcutManager->bind(m_editor->zoomInAction(), PitchLabelerKeys::ShortcutZoomIn, QStringLiteral("Zoom In"), QStringLiteral("View"));
            m_shortcutManager->bind(m_editor->zoomOutAction(), PitchLabelerKeys::ShortcutZoomOut, QStringLiteral("Zoom Out"), QStringLiteral("View"));
            m_shortcutManager->bind(m_editor->zoomResetAction(), PitchLabelerKeys::ShortcutZoomReset, QStringLiteral("Reset Zoom"), QStringLiteral("View"));
            m_shortcutManager->bind(m_editor->abCompareAction(), PitchLabelerKeys::ShortcutABCompare, QStringLiteral("A/B Compare"), QStringLiteral("Tools"));
            m_shortcutManager->applyAll();

            return bar;
        }

        QWidget *PitchLabelerPage::createStatusBarContent(QWidget *parent) {
            auto *container = new QWidget(parent);
            auto *layout = new QHBoxLayout(container);
            layout->setContentsMargins(0, 0, 0, 0);

            auto *statusFile = new QLabel(QStringLiteral("未加载文件"));
            statusFile->setMinimumWidth(160);
            layout->addWidget(statusFile);

            auto *statusPosition = new QLabel("00:00.000");
            statusPosition->setMinimumWidth(100);
            layout->addWidget(statusPosition);

            auto *statusZoom = new QLabel("100%");
            statusZoom->setMinimumWidth(60);
            layout->addWidget(statusZoom);

            auto *statusTool = new QLabel(QStringLiteral("工具: 选择"));
            statusTool->setMinimumWidth(100);
            layout->addWidget(statusTool);

            layout->addStretch();

            auto *statusNotes = new QLabel(QStringLiteral("音符数: 0"));
            statusNotes->setMinimumWidth(80);
            layout->addWidget(statusNotes);

            connect(this, &PitchLabelerPage::fileStatusChanged, statusFile, [statusFile](const QString &name) {
                statusFile->setText(name.isEmpty() ? QStringLiteral("未加载文件") : name);
            });
            connect(m_editor, &PitchEditor::positionChanged, statusPosition, [statusPosition](double sec) {
                int minutes = static_cast<int>(sec) / 60;
                double seconds = sec - minutes * 60;
                statusPosition->setText(
                    QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 6, 'f', 3, QChar('0')));
            });
            connect(m_editor, &PitchEditor::zoomChanged, statusZoom, [statusZoom](int percent) {
                statusZoom->setText(QString::number(percent) + "%");
            });
            connect(m_editor, &PitchEditor::noteCountChanged, statusNotes, [statusNotes](int count) {
                statusNotes->setText(QStringLiteral("音符数: ") + QString::number(count));
            });
            connect(m_editor, &PitchEditor::toolModeChanged, statusTool, [statusTool](int mode) {
                switch (mode) {
                    case 0: statusTool->setText(QStringLiteral("工具: 选择")); break;
                    case 1: statusTool->setText(QStringLiteral("工具: 颤音调制")); break;
                    case 2: statusTool->setText(QStringLiteral("工具: 音高偏移")); break;
                    default: break;
                }
            });

            return container;
        }

    } // namespace pitchlabeler
} // namespace dstools
