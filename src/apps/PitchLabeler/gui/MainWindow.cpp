#include "MainWindow.h"

#include "DSFile.h"
#include "../PitchLabelerKeys.h"
#include "ui/FileListPanel.h"
#include "ui/PianoRollView.h"
#include "ui/PropertyPanel.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <dstools/Theme.h>

#include <algorithm>
#include <functional>

namespace dstools {
    namespace pitchlabeler {

        MainWindow::MainWindow(QWidget *parent)
            : QMainWindow(parent), m_playWidget(new dstools::widgets::PlayWidget()) {
            setWindowTitle(QStringLiteral("DiffSinger 音高标注器"));
            setMinimumSize(1024, 680);
            resize(1440, 900);

            buildMenuBar();
            buildToolbar();
            buildCentralLayout();
            buildStatusBar();
            connectSignals();
            applyConfig();

            updateWindowTitle();
        }

        MainWindow::~MainWindow() = default;

        void MainWindow::buildMenuBar() {
            auto *bar = menuBar();

            // ---- 文件 ----
            m_fileMenu = bar->addMenu(QStringLiteral("文件(&F)"));

            m_actOpenDir = new QAction(QStringLiteral("打开工作目录(&O)..."), this);
            m_actOpenDir->setShortcut(QKeySequence("Ctrl+O"));
            m_actOpenDir->setStatusTip(QStringLiteral("打开包含 .ds 文件的目录"));
            m_fileMenu->addAction(m_actOpenDir);

            m_actCloseDir = new QAction(QStringLiteral("关闭工作目录(&C)"), this);
            m_actCloseDir->setStatusTip(QStringLiteral("关闭当前工作目录"));
            m_fileMenu->addAction(m_actCloseDir);

            m_fileMenu->addSeparator();

            m_actSave = new QAction(QStringLiteral("保存(&S)"), this);
            m_actSave->setShortcut(QKeySequence("Ctrl+S"));
            m_actSave->setStatusTip(QStringLiteral("保存当前文件"));
            m_actSave->setEnabled(false);
            m_fileMenu->addAction(m_actSave);

            m_actSaveAll = new QAction(QStringLiteral("全部保存(&L)"), this);
            m_actSaveAll->setShortcut(QKeySequence("Ctrl+Shift+S"));
            m_actSaveAll->setStatusTip(QStringLiteral("保存所有已修改的文件"));
            m_fileMenu->addAction(m_actSaveAll);

            m_fileMenu->addSeparator();

            m_actExit = new QAction(QStringLiteral("退出(&X)"), this);
            m_actExit->setShortcut(QKeySequence("Alt+F4"));
            m_actExit->setStatusTip(QStringLiteral("退出应用程序"));
            m_fileMenu->addAction(m_actExit);

            // ---- 编辑 ----
            m_editMenu = bar->addMenu(QStringLiteral("编辑(&E)"));

            m_actUndo = new QAction(QStringLiteral("撤销(&U)"), this);
            m_actUndo->setShortcut(QKeySequence("Ctrl+Z"));
            m_actUndo->setStatusTip(QStringLiteral("撤销上一步操作"));
            m_actUndo->setEnabled(false);
            m_editMenu->addAction(m_actUndo);

            m_actRedo = new QAction(QStringLiteral("重做(&R)"), this);
            m_actRedo->setShortcut(QKeySequence("Ctrl+Y"));
            m_actRedo->setStatusTip(QStringLiteral("重做上一步撤销的操作"));
            m_actRedo->setEnabled(false);
            m_editMenu->addAction(m_actRedo);

            // ---- 视图 ----
            m_viewMenu = bar->addMenu(QStringLiteral("视图(&V)"));

            m_actZoomIn = new QAction(QStringLiteral("放大(&I)"), this);
            m_actZoomIn->setShortcut(QKeySequence("Ctrl+="));
            m_actZoomIn->setStatusTip(QStringLiteral("水平放大"));
            m_viewMenu->addAction(m_actZoomIn);

            m_actZoomOut = new QAction(QStringLiteral("缩小(&O)"), this);
            m_actZoomOut->setShortcut(QKeySequence("Ctrl+-"));
            m_actZoomOut->setStatusTip(QStringLiteral("水平缩小"));
            m_viewMenu->addAction(m_actZoomOut);

            m_actZoomReset = new QAction(QStringLiteral("重置缩放(&R)"), this);
            m_actZoomReset->setShortcut(QKeySequence("Ctrl+0"));
            m_actZoomReset->setStatusTip(QStringLiteral("重置缩放到默认级别"));
            m_viewMenu->addAction(m_actZoomReset);

            m_viewMenu->addSeparator();

            // Add theme menu
            dstools::Theme::instance().populateThemeMenu(m_viewMenu);

            // ---- 工具 ----
            m_toolsMenu = bar->addMenu(QStringLiteral("工具(&T)"));

            m_actABCompare = new QAction(QStringLiteral("A/B 对比(&B)"), this);
            m_actABCompare->setShortcut(QKeySequence("Ctrl+B"));
            m_actABCompare->setCheckable(true);
            m_actABCompare->setStatusTip(QStringLiteral("切换 A/B 修改前后对比"));
            m_toolsMenu->addAction(m_actABCompare);

            // ---- 帮助 ----
            m_helpMenu = bar->addMenu(QStringLiteral("帮助(&H)"));

            m_actAbout = new QAction(QStringLiteral("关于(&A)"), this);
            m_actAbout->setStatusTip(QStringLiteral("关于 DiffSinger 音高标注器"));
            m_helpMenu->addAction(m_actAbout);
        }

        void MainWindow::buildToolbar() {
            // Toolbar is created inline in buildCentralLayout
        }

        void MainWindow::buildCentralLayout() {
            // Outer horizontal splitter: FileList | Main area
            m_outerSplitter = new QSplitter(Qt::Horizontal);
            setCentralWidget(m_outerSplitter);

            // Left: File list panel (occupies full height)
            m_fileListPanel = new ui::FileListPanel();
            m_fileListPanel->setMinimumWidth(160);
            m_fileListPanel->setMaximumWidth(280);
            m_outerSplitter->addWidget(m_fileListPanel);

            // Right: vertical stack
            m_rightSplitter = new QSplitter(Qt::Vertical);
            m_outerSplitter->addWidget(m_rightSplitter);

            // Right stack: stacked widget (empty state / content)
            m_mainStack = new QStackedWidget();

            // Page 0: Empty state
            m_emptyPage = new QWidget();
            auto *emptyLayout = new QVBoxLayout(m_emptyPage);
            emptyLayout->setAlignment(Qt::AlignCenter);
            auto *emptyLabel = new QLabel(QStringLiteral("请先打开文件"));
            emptyLabel->setAlignment(Qt::AlignCenter);
            emptyLabel->setStyleSheet("color: #6F737A; font-size: 14px; padding: 40px;");
            emptyLayout->addWidget(emptyLabel);
            m_mainStack->addWidget(m_emptyPage);

            // Page 1: Content with Toolbar + PianoRoll + PropertyPanel
            auto *contentWidget = new QWidget();
            auto *contentLayout = new QVBoxLayout(contentWidget);
            contentLayout->setContentsMargins(0, 0, 0, 0);
            contentLayout->setSpacing(0);

            // Toolbar
            auto *toolbar = new QToolBar();
            toolbar->setMovable(false);
            toolbar->setFloatable(false);
            toolbar->setIconSize(QSize(18, 18));
            toolbar->setStyleSheet(
                "QToolBar { background: #2A2A36; border-bottom: 1px solid #33333E; spacing: 4px; padding: 2px 6px; }");

            // ---- Playback controls ----
            m_actPlayPause = new QAction(QStringLiteral("播放"), this);
            m_actPlayPause->setStatusTip(QStringLiteral("播放/暂停 (Space)"));
            toolbar->addAction(m_actPlayPause);

            m_actStop = new QAction(QStringLiteral("停止"), this);
            m_actStop->setStatusTip(QStringLiteral("停止 (Escape)"));
            toolbar->addAction(m_actStop);

            toolbar->addSeparator();

            // ---- Tool mode buttons ----
            m_btnToolSelect = new QToolButton();
            m_btnToolSelect->setText(QStringLiteral("选择"));
            m_btnToolSelect->setToolTip(QStringLiteral("选择工具 (V)"));
            m_btnToolSelect->setCheckable(true);
            m_btnToolSelect->setChecked(true);
            toolbar->addWidget(m_btnToolSelect);

            m_btnToolModulation = new QToolButton();
            m_btnToolModulation->setText(QStringLiteral("颤音调制"));
            m_btnToolModulation->setToolTip(QStringLiteral("颤音调制工具 (M)"));
            m_btnToolModulation->setCheckable(true);
            toolbar->addWidget(m_btnToolModulation);

            m_btnToolDrift = new QToolButton();
            m_btnToolDrift->setText(QStringLiteral("音高偏移"));
            m_btnToolDrift->setToolTip(QStringLiteral("音高偏移工具 (D)"));
            m_btnToolDrift->setCheckable(true);
            toolbar->addWidget(m_btnToolDrift);

            // Exclusive tool mode group
            m_toolModeGroup = new QActionGroup(this);
            auto *actSelect = new QAction(QStringLiteral("选择"), m_toolModeGroup);
            actSelect->setCheckable(true);
            actSelect->setChecked(true);
            auto *actModulation = new QAction(QStringLiteral("颤音调制"), m_toolModeGroup);
            actModulation->setCheckable(true);
            auto *actDrift = new QAction(QStringLiteral("音高偏移"), m_toolModeGroup);
            actDrift->setCheckable(true);
            m_toolModeGroup->setExclusive(true);

            // Sync tool buttons with action group
            connect(m_btnToolSelect, &QToolButton::clicked, this, [this]() {
                setToolMode(ui::ToolSelect);
            });
            connect(m_btnToolModulation, &QToolButton::clicked, this, [this]() {
                setToolMode(ui::ToolModulation);
            });
            connect(m_btnToolDrift, &QToolButton::clicked, this, [this]() {
                setToolMode(ui::ToolDrift);
            });

            toolbar->addSeparator();

            // A/B comparison toggle
            toolbar->addAction(m_actABCompare);

            toolbar->addSeparator();

            // Zoom controls
            toolbar->addAction(m_actZoomIn);
            toolbar->addAction(m_actZoomOut);
            toolbar->addAction(m_actZoomReset);

            contentLayout->addWidget(toolbar);

            // Piano roll view (takes remaining space)
            m_pianoRoll = new ui::PianoRollView();
            contentLayout->addWidget(m_pianoRoll, 1);

            // Property panel (collapsible at bottom)
            m_propertyPanel = new ui::PropertyPanel();
            m_propertyPanel->setMinimumHeight(28);
            m_propertyPanel->setMaximumHeight(200);
            contentLayout->addWidget(m_propertyPanel);

            m_mainStack->addWidget(contentWidget);

            m_rightSplitter->addWidget(m_mainStack);

            // Initial sizes
            m_outerSplitter->setSizes({200, 1240});
            m_rightSplitter->setSizes({600, 160});

            // Set stretch factors
            m_rightSplitter->setStretchFactor(0, 1); // content gets more space
            m_rightSplitter->setStretchFactor(1, 0); // property panel fixed
        }

        void MainWindow::buildStatusBar() {
            auto *sb = statusBar();

            m_statusFile = new QLabel(QStringLiteral("未加载文件"));
            m_statusFile->setMinimumWidth(160);
            sb->addWidget(m_statusFile);

            m_statusPosition = new QLabel("00:00.000");
            m_statusPosition->setMinimumWidth(100);
            sb->addWidget(m_statusPosition);

            m_statusZoom = new QLabel("100%");
            m_statusZoom->setMinimumWidth(60);
            sb->addWidget(m_statusZoom);

            m_statusTool = new QLabel(QStringLiteral("工具: 选择"));
            m_statusTool->setMinimumWidth(100);
            sb->addWidget(m_statusTool);

            m_statusNotes = new QLabel(QStringLiteral("音符数: 0"));
            m_statusNotes->setMinimumWidth(80);
            sb->addPermanentWidget(m_statusNotes);
        }

        void MainWindow::connectSignals() {
            // File menu
            connect(m_actOpenDir, &QAction::triggered, this, &MainWindow::openDirectory);
            connect(m_actCloseDir, &QAction::triggered, this, &MainWindow::closeDirectory);
            connect(m_actSave, &QAction::triggered, this, &MainWindow::saveFile);
            connect(m_actSaveAll, &QAction::triggered, this, &MainWindow::saveAllFiles);
            connect(m_actExit, &QAction::triggered, this, &QMainWindow::close);

            // Edit menu
            connect(m_actUndo, &QAction::triggered, this, &MainWindow::onUndo);
            connect(m_actRedo, &QAction::triggered, this, &MainWindow::onRedo);

            // View menu
            connect(m_actZoomIn, &QAction::triggered, this, &MainWindow::onZoomIn);
            connect(m_actZoomOut, &QAction::triggered, this, &MainWindow::onZoomOut);
            connect(m_actZoomReset, &QAction::triggered, this, &MainWindow::onZoomReset);

            // Tools menu
            connect(m_actABCompare, &QAction::toggled, this, [this](bool checked) {
                m_abComparisonActive = checked;
                m_pianoRoll->setABComparisonActive(checked);
            });

            // Help menu
            connect(m_actAbout, &QAction::triggered, this, []() {
                QMessageBox::about(nullptr, QStringLiteral("关于 DiffSinger 音高标注器"),
                                   QStringLiteral("DiffSinger 音高标注器\n版本 0.1.0\n\n"
                                   "DiffSinger 数据标注工作流的音高标注编辑器。"));
            });

            // File list panel
            connect(m_fileListPanel, &ui::FileListPanel::fileSelected, this, &MainWindow::loadFile);

            // Piano roll signals
            connect(m_pianoRoll, &ui::PianoRollView::noteSelected, this, &MainWindow::onNoteSelected);
            connect(m_pianoRoll, &ui::PianoRollView::positionClicked, this, &MainWindow::onPositionClicked);
            connect(m_pianoRoll, &ui::PianoRollView::fileEdited, this, [this]() {
                if (m_currentFile) {
                    m_currentFile->markModified();
                    updateWindowTitle();
                    updateStatusBar();
                    m_fileListPanel->setFileModified(m_currentFilePath, true);
                }
            });

            // Note editing from context menu
            connect(m_pianoRoll, &ui::PianoRollView::noteDeleteRequested, this, [this](const std::vector<int> &indices) {
                if (!m_currentFile || indices.empty()) return;
                // Delete in reverse order to keep indices valid
                std::vector<int> sorted = indices;
                std::sort(sorted.begin(), sorted.end(), std::greater<int>());
                for (int idx : sorted) {
                    if (idx >= 0 && idx < static_cast<int>(m_currentFile->notes.size())) {
                        m_currentFile->notes.erase(m_currentFile->notes.begin() + idx);
                    }
                }
                m_currentFile->recomputeNoteStarts();
                m_currentFile->markModified();
                m_pianoRoll->setDSFile(m_currentFile);
                updateWindowTitle();
                m_fileListPanel->setFileModified(m_currentFilePath, true);
            });
            connect(m_pianoRoll, &ui::PianoRollView::noteGlideChanged, this, [this](int idx, const QString &glide) {
                if (!m_currentFile || idx < 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
                m_currentFile->notes[idx].glide = glide;
                m_currentFile->markModified();
                m_pianoRoll->update();
                updateWindowTitle();
                m_fileListPanel->setFileModified(m_currentFilePath, true);
            });
            connect(m_pianoRoll, &ui::PianoRollView::noteSlurToggled, this, [this](int idx) {
                if (!m_currentFile || idx < 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
                m_currentFile->notes[idx].slur = m_currentFile->notes[idx].slur ? 0 : 1;
                m_currentFile->markModified();
                m_pianoRoll->update();
                updateWindowTitle();
                m_fileListPanel->setFileModified(m_currentFilePath, true);
            });
            connect(m_pianoRoll, &ui::PianoRollView::noteRestToggled, this, [this](int idx) {
                if (!m_currentFile || idx < 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
                auto &note = m_currentFile->notes[idx];
                if (note.isRest()) {
                    // Find nearest non-rest note name to assign
                    for (int off = 1; off < static_cast<int>(m_currentFile->notes.size()); ++off) {
                        for (int j : {idx - off, idx + off}) {
                            if (j >= 0 && j < static_cast<int>(m_currentFile->notes.size()) &&
                                !m_currentFile->notes[j].isRest()) {
                                note.name = m_currentFile->notes[j].name;
                                goto done;
                            }
                        }
                    }
                    note.name = "C4"; // fallback
                    done:;
                } else {
                    note.name = "rest";
                }
                m_currentFile->markModified();
                m_pianoRoll->update();
                updateWindowTitle();
                m_fileListPanel->setFileModified(m_currentFilePath, true);
            });
            connect(m_pianoRoll, &ui::PianoRollView::noteMergeLeft, this, [this](int idx) {
                if (!m_currentFile || idx <= 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
                // Merge slur into left neighbor by extending left's duration
                auto &left = m_currentFile->notes[idx - 1];
                auto &cur = m_currentFile->notes[idx];
                left.duration += cur.duration;
                m_currentFile->notes.erase(m_currentFile->notes.begin() + idx);
                m_currentFile->recomputeNoteStarts();
                m_currentFile->markModified();
                m_pianoRoll->setDSFile(m_currentFile);
                updateWindowTitle();
                m_fileListPanel->setFileModified(m_currentFilePath, true);
            });

            // Tool mode changed from piano roll
            connect(m_pianoRoll, &ui::PianoRollView::toolModeChanged, this, [this](int mode) {
                auto toolMode = static_cast<ui::ToolMode>(mode);
                m_btnToolSelect->setChecked(toolMode == ui::ToolSelect);
                m_btnToolModulation->setChecked(toolMode == ui::ToolModulation);
                m_btnToolDrift->setChecked(toolMode == ui::ToolDrift);

                switch (toolMode) {
                    case ui::ToolSelect:
                        m_statusTool->setText(QStringLiteral("工具: 选择"));
                        break;
                    case ui::ToolModulation:
                        m_statusTool->setText(QStringLiteral("工具: 颤音调制"));
                        break;
                    case ui::ToolDrift:
                        m_statusTool->setText(QStringLiteral("工具: 音高偏移"));
                        break;
                }
            });

            // Playback (toolbar actions)
            connect(m_actPlayPause, &QAction::triggered, this, &MainWindow::onPlayPause);
            connect(m_actStop, &QAction::triggered, this, &MainWindow::onStop);

            // Playback (widget signals)
            connect(m_playWidget, &dstools::widgets::PlayWidget::playheadChanged, this, &MainWindow::updatePlayheadPosition);
        }

        void MainWindow::applyConfig() {
            // Apply saved shortcuts
            m_actOpenDir->setShortcut(m_settings.shortcut(PitchLabelerKeys::ShortcutOpen));
            m_actSave->setShortcut(QKeySequence("Ctrl+S"));
            m_actSaveAll->setShortcut(QKeySequence("Ctrl+Shift+S"));

            // Load piano roll display options
            m_pianoRoll->loadConfig(m_settings);

            // Restore last directory
            if (const QString savedDir = m_settings.get(PitchLabelerKeys::LastDir);
                !savedDir.isEmpty() && QDir(savedDir).exists()) {
                m_workingDirectory = savedDir;
                m_fileListPanel->setDirectory(savedDir);
            }
        }

        void MainWindow::openDirectory() {
            QString dir =
                QFileDialog::getExistingDirectory(this, QStringLiteral("打开工作目录"), "", QFileDialog::ShowDirsOnly);

            if (dir.isEmpty())
                return;

            m_workingDirectory = dir;
            m_fileListPanel->setDirectory(dir);
            m_settings.set(PitchLabelerKeys::LastDir, dir);
            updateWindowTitle();
            statusBar()->showMessage(QStringLiteral("已打开: ") + dir, 3000);
        }

        void MainWindow::closeDirectory() {
            // Save any unsaved files first
            if (m_currentFile && m_currentFile->modified) {
                auto ret = QMessageBox::question(this, QStringLiteral("未保存的更改"),
                                                 QStringLiteral("关闭前是否保存？"),
                                                 QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
                if (ret == QMessageBox::Save) {
                    saveFile();
                } else if (ret == QMessageBox::Cancel) {
                    return;
                }
            }

            // Save file list progress before clearing
            m_fileListPanel->saveState();

            m_workingDirectory.clear();
            m_currentFilePath.clear();
            m_currentFile.reset();

            // Switch to empty state
            m_mainStack->setCurrentIndex(0);
            m_fileListPanel->clear();
            m_pianoRoll->clear();
            m_propertyPanel->clear();

            updateWindowTitle();
            updateStatusBar();
        }

        void MainWindow::loadFile(const QString &path) {
            auto [ds, error] = DSFile::load(path);
            if (!error.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("错误"),
                                     QStringLiteral("加载文件失败: ") + error);
                return;
            }

            m_currentFile = ds;
            m_currentFilePath = path;

            // Switch from empty state to file content
            m_mainStack->setCurrentIndex(1);

            // Update UI
            m_pianoRoll->setDSFile(ds);
            m_propertyPanel->setDSFile(ds);

            // Enable save
            m_actSave->setEnabled(true);

            // Find and load audio file
            QFileInfo fi(path);
            QString audioPath = fi.absolutePath() + "/" + fi.completeBaseName();
            QStringList audioExts = {"wav", "mp3", "m4a", "flac"};
            for (const QString &ext : audioExts) {
                QString testPath = audioPath + "." + ext;
                if (QFile::exists(testPath)) {
                    m_playWidget->openFile(testPath);
                    break;
                }
            }

            // Store original F0 for A/B comparison
            m_originalF0 = ds->f0.values;

            updateWindowTitle();
            updateStatusBar();

            emit fileLoaded(path);
        }

        bool MainWindow::saveFile() {
            if (!m_currentFile)
                return false;

            auto [ok, error] = m_currentFile->save();
            if (!ok) {
                QMessageBox::warning(this, QStringLiteral("错误"),
                                     QStringLiteral("保存失败: ") + error);
                return false;
            }

            updateWindowTitle();
            updateStatusBar();
            statusBar()->showMessage(QStringLiteral("已保存: ") + QFileInfo(m_currentFilePath).fileName(), 3000);
            m_fileListPanel->setFileSaved(m_currentFilePath);
            emit fileSaved(m_currentFilePath);
            return true;
        }

        bool MainWindow::saveAllFiles() {
            if (m_currentFile && m_currentFile->modified) {
                return saveFile();
            }
            return true;
        }

        void MainWindow::updateWindowTitle() {
            QString title = QStringLiteral("DiffSinger 音高标注器");
            if (!m_currentFilePath.isEmpty()) {
                QString fn = QFileInfo(m_currentFilePath).fileName();
                title = fn + (m_currentFile && m_currentFile->modified ? " *" : "") + " - " + title;
            }
            setWindowTitle(title);
        }

        void MainWindow::onUndo() {
            statusBar()->showMessage(QStringLiteral("撤销功能尚未实现"), 2000);
        }

        void MainWindow::onRedo() {
            statusBar()->showMessage(QStringLiteral("重做功能尚未实现"), 2000);
        }

        void MainWindow::updateUndoRedoState() {
            // TODO: Check undo manager state
        }

        void MainWindow::onZoomIn() {
            m_pianoRoll->zoomIn();
            updateZoomStatus();
        }

        void MainWindow::onZoomOut() {
            m_pianoRoll->zoomOut();
            updateZoomStatus();
        }

        void MainWindow::onZoomReset() {
            m_pianoRoll->resetZoom();
            updateZoomStatus();
        }

        void MainWindow::updateZoomStatus() {
            int zoom = m_pianoRoll->getZoomPercent();
            m_statusZoom->setText(QString::number(zoom) + "%");
        }

        void MainWindow::onPlayPause() {
            m_playWidget->setPlaying(!m_playWidget->isPlaying());
        }

        void MainWindow::onStop() {
            m_playWidget->setPlaying(false);
        }

        void MainWindow::updatePlayheadPosition(double sec) {
            m_pianoRoll->setPlayheadTime(sec);

            int minutes = static_cast<int>(sec) / 60;
            double seconds = sec - minutes * 60;
            m_statusPosition->setText(
                QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 6, 'f', 3, QChar('0')));
        }

        void MainWindow::updatePlaybackState() {
            // Update play button icon if needed
        }

        void MainWindow::setToolMode(ui::ToolMode mode) {
            m_pianoRoll->setToolMode(mode);
            m_btnToolSelect->setChecked(mode == ui::ToolSelect);
            m_btnToolModulation->setChecked(mode == ui::ToolModulation);
            m_btnToolDrift->setChecked(mode == ui::ToolDrift);
        }

        void MainWindow::onNoteSelected(int noteIndex) {
            if (!m_currentFile)
                return;

            if (noteIndex >= 0 && noteIndex < static_cast<int>(m_currentFile->notes.size())) {
                m_propertyPanel->setSelectedNote(noteIndex);
            }
        }

        void MainWindow::onPositionClicked(double time, double midi) {
            int minutes = static_cast<int>(time) / 60;
            double seconds = time - minutes * 60;
            m_statusPosition->setText(
                QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 6, 'f', 3, QChar('0')));
        }

        void MainWindow::onNextFile() {
            m_fileListPanel->selectNextFile();
        }

        void MainWindow::onPrevFile() {
            m_fileListPanel->selectPrevFile();
        }

        void MainWindow::updateStatusBar() {
            if (!m_currentFilePath.isEmpty()) {
                m_statusFile->setText(QFileInfo(m_currentFilePath).fileName());
            } else {
                m_statusFile->setText(QStringLiteral("未加载文件"));
            }

            if (m_currentFile) {
                m_statusNotes->setText(QStringLiteral("音符数: ") + QString::number(m_currentFile->getNoteCount()));
            } else {
                m_statusNotes->setText(QStringLiteral("音符数: 0"));
            }
        }

        void MainWindow::reloadCurrentFile() {
            if (!m_currentFilePath.isEmpty()) {
                loadFile(m_currentFilePath);
            }
        }

        void MainWindow::closeEvent(QCloseEvent *event) {
            // Save config
            if (!m_workingDirectory.isEmpty()) {
                m_settings.set(PitchLabelerKeys::LastDir, m_workingDirectory);
            }

            // Save piano roll display options
            m_pianoRoll->pullConfig(m_settings);

            // Save file list state (progress tracking)
            m_fileListPanel->saveState();

            // Check for unsaved changes
            if (m_currentFile && m_currentFile->modified) {
                auto ret = QMessageBox::question(this, QStringLiteral("未保存的更改"),
                                                 QStringLiteral("关闭前是否保存？"),
                                                 QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
                if (ret == QMessageBox::Save) {
                    if (!saveFile()) {
                        event->ignore();
                        return;
                    }
                } else if (ret == QMessageBox::Cancel) {
                    event->ignore();
                    return;
                }
            }
            event->accept();
        }

        void MainWindow::keyPressEvent(QKeyEvent *event) {
            // Tool mode shortcuts
            if (event->modifiers() == Qt::NoModifier) {
                switch (event->key()) {
                    case Qt::Key_V:
                        setToolMode(ui::ToolSelect);
                        event->accept();
                        return;
                    case Qt::Key_M:
                        setToolMode(ui::ToolModulation);
                        event->accept();
                        return;
                    case Qt::Key_D:
                        setToolMode(ui::ToolDrift);
                        event->accept();
                        return;
                    case Qt::Key_Space:
                        onPlayPause();
                        event->accept();
                        return;
                    case Qt::Key_Escape:
                        onStop();
                        event->accept();
                        return;
                    case Qt::Key_PageUp:
                        onPrevFile();
                        event->accept();
                        return;
                    case Qt::Key_PageDown:
                        onNextFile();
                        event->accept();
                        return;
                    default:
                        break;
                }
            }

            if (event->modifiers() == Qt::ShiftModifier) {
                switch (event->key()) {
                    case Qt::Key_Space:
                        onStop();
                        event->accept();
                        return;
                    default:
                        break;
                }
            }

            QMainWindow::keyPressEvent(event);
        }

    } // namespace pitchlabeler
} // namespace dstools
