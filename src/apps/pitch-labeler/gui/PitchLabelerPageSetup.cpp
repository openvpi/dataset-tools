#include "PitchLabelerPage.h"

#include "DSFile.h"
#include "PitchFileService.h"
#include "../PitchLabelerKeys.h"
#include "ui/FileListPanel.h"
#include "ui/PianoRollView.h"
#include "ui/PropertyPanel.h"
#include "ui/commands/NoteCommands.h"

#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QProxyStyle>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QIcon>

#include <dstools/ShortcutManager.h>
#include <dsfw/Theme.h>

namespace {
    /// Style override: left-click anywhere on the slider groove jumps directly
    /// to that position (instead of paging). This lets the user click+drag
    /// seamlessly because QSlider enters its normal drag tracking after the jump.
    class JumpClickStyle : public QProxyStyle {
    public:
        using QProxyStyle::QProxyStyle;
        int styleHint(StyleHint hint, const QStyleOption *option = nullptr,
                      const QWidget *widget = nullptr,
                      QStyleHintReturn *returnData = nullptr) const override {
            if (hint == SH_Slider_AbsoluteSetButtons)
                return Qt::LeftButton;
            return QProxyStyle::styleHint(hint, option, widget, returnData);
        }
    };
}

namespace dstools {
    namespace pitchlabeler {

        void PitchLabelerPage::buildActions() {
            // Edit actions
            m_actSave = new QAction(QStringLiteral("保存(&S)"), this);
            m_actSave->setStatusTip(QStringLiteral("保存当前文件"));
            m_actSave->setEnabled(false);

            m_actSaveAll = new QAction(QStringLiteral("全部保存(&L)"), this);
            m_actSaveAll->setStatusTip(QStringLiteral("保存所有已修改的文件"));

            m_actUndo = new QAction(QStringLiteral("撤销(&U)"), this);
            m_actUndo->setStatusTip(QStringLiteral("撤销上一步操作"));
            m_actUndo->setEnabled(false);

            m_actRedo = new QAction(QStringLiteral("重做(&R)"), this);
            m_actRedo->setStatusTip(QStringLiteral("重做上一步撤销的操作"));
            m_actRedo->setEnabled(false);

            // View actions
            m_actZoomIn = new QAction(QStringLiteral("放大(&I)"), this);
            m_actZoomIn->setStatusTip(QStringLiteral("水平放大"));

            m_actZoomOut = new QAction(QStringLiteral("缩小(&O)"), this);
            m_actZoomOut->setStatusTip(QStringLiteral("水平缩小"));

            m_actZoomReset = new QAction(QStringLiteral("重置缩放(&R)"), this);
            m_actZoomReset->setStatusTip(QStringLiteral("重置缩放到默认级别"));

            // Tool actions
            m_actABCompare = new QAction(QStringLiteral("A/B 对比(&B)"), this);
            m_actABCompare->setCheckable(true);
            m_actABCompare->setStatusTip(QStringLiteral("切换 A/B 修改前后对比"));
        }

        void PitchLabelerPage::buildLayout() {
            auto *pageLayout = new QVBoxLayout(this);
            pageLayout->setContentsMargins(0, 0, 0, 0);
            pageLayout->setSpacing(0);

            // Outer horizontal splitter: FileList | Main area
            m_outerSplitter = new QSplitter(Qt::Horizontal);
            pageLayout->addWidget(m_outerSplitter);

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

            // Toolbar (as a regular QToolBar widget, not attached to QMainWindow)
            auto *toolbar = new QToolBar();
            toolbar->setMovable(false);
            toolbar->setFloatable(false);
            toolbar->setIconSize(QSize(18, 18));
            toolbar->setStyleSheet(
                "QToolBar { background: #2A2A36; border-bottom: 1px solid #33333E; spacing: 4px; padding: 2px 6px; }");

            // ---- Styled tool mode buttons (LEFT side) ----
            static const QString toolBtnStyle = QStringLiteral(
                "QToolButton { padding: 4px 12px; border-radius: 3px; font-size: 12px; }"
                "QToolButton:checked { background-color: #4A90D9; color: white; }"
                "QToolButton:!checked { background: transparent; color: #9898A8; }");

            m_btnToolSelect = new QToolButton();
            m_btnToolSelect->setText(QStringLiteral("↑ 选择"));
            m_btnToolSelect->setToolTip(QStringLiteral("选择工具 (V)"));
            m_btnToolSelect->setCheckable(true);
            m_btnToolSelect->setChecked(true);
            m_btnToolSelect->setStyleSheet(toolBtnStyle);
            toolbar->addWidget(m_btnToolSelect);

            m_btnToolModulation = new QToolButton();
            m_btnToolModulation->setText(QStringLiteral("≡ 颤音调制"));
            m_btnToolModulation->setToolTip(QStringLiteral("颤音调制工具 (M)"));
            m_btnToolModulation->setCheckable(true);
            m_btnToolModulation->setStyleSheet(toolBtnStyle);
            toolbar->addWidget(m_btnToolModulation);

            m_btnToolDrift = new QToolButton();
            m_btnToolDrift->setText(QStringLiteral("↕ 音高偏移"));
            m_btnToolDrift->setToolTip(QStringLiteral("音高偏移工具 (D)"));
            m_btnToolDrift->setCheckable(true);
            m_btnToolDrift->setStyleSheet(toolBtnStyle);
            toolbar->addWidget(m_btnToolDrift);

            m_btnToolAudition = new QToolButton();
            m_btnToolAudition->setText(QStringLiteral("✎ 试听"));
            m_btnToolAudition->setToolTip(QStringLiteral("试听工具"));
            m_btnToolAudition->setCheckable(true);
            m_btnToolAudition->setStyleSheet(toolBtnStyle);
            toolbar->addWidget(m_btnToolAudition);

            // ---- Spacer to push playback controls to right ----
            auto *toolbarSpacer = new QWidget();
            toolbarSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            toolbar->addWidget(toolbarSpacer);

            // ---- Playback controls (RIGHT side) ----
            m_actPlayPause = new QAction(this);
            m_actPlayPause->setIcon(QIcon(":/icons/play.svg"));
            m_actPlayPause->setStatusTip(QStringLiteral("播放/暂停 (Space)"));
            toolbar->addAction(m_actPlayPause);

            m_actStop = new QAction(this);
            m_actStop->setIcon(QIcon(":/icons/stop.svg"));
            m_actStop->setStatusTip(QStringLiteral("停止 (Escape)"));
            toolbar->addAction(m_actStop);

            toolbar->addSeparator();

            // ---- Waveform display toggle ----
            m_btnWaveformToggle = new QToolButton();
            m_btnWaveformToggle->setIcon(QIcon(":/icons/audio.svg"));
            m_btnWaveformToggle->setToolTip(QStringLiteral("切换波形显示"));
            m_btnWaveformToggle->setCheckable(true);
            m_btnWaveformToggle->setChecked(true);
            toolbar->addWidget(m_btnWaveformToggle);

            // ---- Volume control ----
            m_volumeSlider = new QSlider(Qt::Horizontal);
            m_volumeSlider->setRange(0, 100);
            m_volumeSlider->setValue(100);
            m_volumeSlider->setFixedWidth(80);
            m_volumeSlider->setStyleSheet(
                "QSlider { margin: 0 4px; }"
                "QSlider::groove:horizontal { background: #33333E; height: 4px; border-radius: 2px; }"
                "QSlider::handle:horizontal { background: #4A90D9; width: 10px; margin: -3px 0; border-radius: 5px; }");
            toolbar->addWidget(m_volumeSlider);

            m_volumeLabel = new QLabel(QStringLiteral("100%"));
            m_volumeLabel->setStyleSheet("color: #9898A8; font-size: 11px; margin-left: 2px;");
            toolbar->addWidget(m_volumeLabel);

            connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
                m_volumeLabel->setText(QString::number(value) + QStringLiteral("%"));
            });

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

            contentLayout->addWidget(toolbar);

            // Playback progress widget above piano roll
            m_playbackProgressWidget = new QWidget();
            auto *progressLayout = new QHBoxLayout(m_playbackProgressWidget);
            progressLayout->setContentsMargins(8, 4, 8, 4);
            progressLayout->setSpacing(8);

            m_progressCurrentTime = new QLabel("00:00.000");
            m_progressCurrentTime->setStyleSheet("color: #9898A8; font-size: 11px; font-family: Consolas;");
            m_progressCurrentTime->setMinimumWidth(70);
            progressLayout->addWidget(m_progressCurrentTime);

            m_playbackProgressSlider = new QSlider(Qt::Horizontal);
            m_jumpClickStyle = std::make_unique<JumpClickStyle>(m_playbackProgressSlider->style());
            m_playbackProgressSlider->setStyle(m_jumpClickStyle.get());
            m_playbackProgressSlider->setRange(0, 10000); // 0-10s in ms by default
            m_playbackProgressSlider->setValue(0);
            m_playbackProgressSlider->setFixedHeight(16);
            m_playbackProgressSlider->setTracking(true);
            m_playbackProgressSlider->setStyleSheet(
                "QSlider { margin: 0 4px; }"
                "QSlider::groove:horizontal { background: #33333E; height: 4px; border-radius: 2px; }"
                "QSlider::handle:horizontal { background: #4A90D9; width: 10px; margin: -3px 0; border-radius: 5px; }"
                "QSlider::sub-page:horizontal { background: #4A90D9; height: 4px; border-radius: 2px; }"
            );
            progressLayout->addWidget(m_playbackProgressSlider, 1);

            m_progressTotalTime = new QLabel("00:00.000");
            m_progressTotalTime->setStyleSheet("color: #9898A8; font-size: 11px; font-family: Consolas;");
            m_progressTotalTime->setMinimumWidth(70);
            progressLayout->addWidget(m_progressTotalTime);

            contentLayout->addWidget(m_playbackProgressWidget);

            // Piano roll view (takes remaining space)
            m_pianoRoll = new ui::PianoRollView();
            m_pianoRoll->setViewportController(m_viewport);
            m_pianoRoll->setUndoStack(m_undoStack);
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

        void PitchLabelerPage::connectSignals() {
            // Edit actions
            connect(m_actSave, &QAction::triggered, this, &PitchLabelerPage::saveFile);
            connect(m_actSaveAll, &QAction::triggered, this, &PitchLabelerPage::saveAllFiles);
            connect(m_actUndo, &QAction::triggered, this, &PitchLabelerPage::onUndo);
            connect(m_actRedo, &QAction::triggered, this, &PitchLabelerPage::onRedo);

            // Undo stack state tracking
            connect(m_undoStack, &QUndoStack::canUndoChanged, m_actUndo, &QAction::setEnabled);
            connect(m_undoStack, &QUndoStack::canRedoChanged, m_actRedo, &QAction::setEnabled);

            // View actions
            connect(m_actZoomIn, &QAction::triggered, this, &PitchLabelerPage::onZoomIn);
            connect(m_actZoomOut, &QAction::triggered, this, &PitchLabelerPage::onZoomOut);
            connect(m_actZoomReset, &QAction::triggered, this, &PitchLabelerPage::onZoomReset);

            // Tools
            connect(m_actABCompare, &QAction::toggled, this, [this](bool checked) {
                m_abComparisonActive = checked;
                m_pianoRoll->setABComparisonActive(checked);
            });

            // File list panel
            connect(m_fileListPanel, &ui::FileListPanel::fileSelected, this, &PitchLabelerPage::loadFile);

            // Piano roll signals
            connect(m_pianoRoll, &ui::PianoRollView::noteSelected, this, &PitchLabelerPage::onNoteSelected);
            connect(m_pianoRoll, &ui::PianoRollView::positionClicked, this, &PitchLabelerPage::onPositionClicked);
            connect(m_pianoRoll, &ui::PianoRollView::rulerClicked, this, [this](double timeSec) {
                if (m_playWidget) {
                    m_playWidget->seek(timeSec);
                    updatePlayheadPosition(timeSec);
                }
            });
            connect(m_pianoRoll, &ui::PianoRollView::fileEdited, this, [this]() {
                if (m_currentFile) {
                    m_fileService->markCurrentFileModified();
                    updateStatusInfo();
                    m_fileListPanel->setFileModified(m_currentFilePath, true);
                    emit modificationChanged(true);
                }
            });

            // Note editing from context menu
            connect(m_pianoRoll, &ui::PianoRollView::noteDeleteRequested, this, [this](const std::vector<int> &indices) {
                if (!m_currentFile || indices.empty()) return;
                m_undoStack->push(new ui::DeleteNotesCommand(m_currentFile, indices));
                m_pianoRoll->setDSFile(m_currentFile);
                m_fileListPanel->setFileModified(m_currentFilePath, true);
                emit modificationChanged(true);
            });
            connect(m_pianoRoll, &ui::PianoRollView::noteGlideChanged, this, [this](int idx, const QString &glide) {
                if (!m_currentFile || idx < 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
                QString oldGlide = m_currentFile->notes[idx].glide;
                m_undoStack->push(new ui::SetNoteGlideCommand(m_currentFile, idx, oldGlide, glide));
                m_pianoRoll->update();
                m_fileListPanel->setFileModified(m_currentFilePath, true);
                emit modificationChanged(true);
            });
            connect(m_pianoRoll, &ui::PianoRollView::noteSlurToggled, this, [this](int idx) {
                if (!m_currentFile || idx < 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
                int oldSlur = m_currentFile->notes[idx].slur;
                m_undoStack->push(new ui::ToggleNoteSlurCommand(m_currentFile, idx, oldSlur));
                m_pianoRoll->update();
                m_fileListPanel->setFileModified(m_currentFilePath, true);
                emit modificationChanged(true);
            });
            connect(m_pianoRoll, &ui::PianoRollView::noteRestToggled, this, [this](int idx) {
                if (!m_currentFile || idx < 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
                QString oldName = m_currentFile->notes[idx].name;
                bool wasRest = m_currentFile->notes[idx].isRest();
                m_undoStack->push(new ui::ToggleNoteRestCommand(m_currentFile, idx, oldName, wasRest));
                m_pianoRoll->update();
                m_fileListPanel->setFileModified(m_currentFilePath, true);
                emit modificationChanged(true);
            });
            connect(m_pianoRoll, &ui::PianoRollView::noteMergeLeft, this, [this](int idx) {
                if (!m_currentFile || idx <= 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
                m_undoStack->push(new ui::MergeNoteLeftCommand(m_currentFile, idx));
                m_pianoRoll->setDSFile(m_currentFile);
                m_fileListPanel->setFileModified(m_currentFilePath, true);
                emit modificationChanged(true);
            });

            // Tool mode changed from piano roll
            connect(m_pianoRoll, &ui::PianoRollView::toolModeChanged, this, [this](int mode) {
                auto toolMode = static_cast<ui::ToolMode>(mode);
                m_btnToolSelect->setChecked(toolMode == ui::ToolSelect);
                m_btnToolModulation->setChecked(toolMode == ui::ToolModulation);
                m_btnToolDrift->setChecked(toolMode == ui::ToolDrift);
                emit toolModeChanged(mode);
            });

            // Playback (toolbar actions)
            connect(m_actPlayPause, &QAction::triggered, this, &PitchLabelerPage::onPlayPause);
            connect(m_actStop, &QAction::triggered, this, &PitchLabelerPage::onStop);

            // Playback (widget signals)
            connect(m_playWidget, &dstools::widgets::PlayWidget::playheadChanged, this, &PitchLabelerPage::updatePlayheadPosition);

            // Seek when user interacts with the playback progress slider
            connect(m_playbackProgressSlider, &QSlider::valueChanged, this, [this](int value) {
                if (!m_playbackProgressSlider->isSliderDown()) return;
                if (!m_currentFile || !m_playWidget) return;
                double sec = value / 1000.0;
                m_playWidget->seek(sec);
                m_pianoRoll->setPlayheadTime(sec);
                updateTimeLabels(sec);
            });
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
            fileMenu->addAction(m_actSave);
            fileMenu->addAction(m_actSaveAll);
            fileMenu->addSeparator();

            auto *actExit = new QAction(QStringLiteral("退出(&X)"), bar);
            actExit->setStatusTip(QStringLiteral("退出应用程序"));
            fileMenu->addAction(actExit);
            connect(actExit, &QAction::triggered, this, [this]() {
                if (auto *w = window()) w->close();
            });

            // ---- 编辑 ----
            auto *editMenu = bar->addMenu(QStringLiteral("编辑(&E)"));
            editMenu->addAction(m_actUndo);
            editMenu->addAction(m_actRedo);

            // ---- 视图 ----
            auto *viewMenu = bar->addMenu(QStringLiteral("视图(&V)"));
            viewMenu->addAction(m_actZoomIn);
            viewMenu->addAction(m_actZoomOut);
            viewMenu->addAction(m_actZoomReset);
            viewMenu->addSeparator();
            dsfw::Theme::instance().populateThemeMenu(viewMenu);

            // ---- 工具 ----
            auto *toolsMenu = bar->addMenu(QStringLiteral("工具(&T)"));
            toolsMenu->addAction(m_actABCompare);
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
            m_shortcutManager->bind(m_actSave, PitchLabelerKeys::ShortcutSave, QStringLiteral("Save"), QStringLiteral("File"));
            m_shortcutManager->bind(m_actSaveAll, PitchLabelerKeys::ShortcutSaveAll, QStringLiteral("Save All"), QStringLiteral("File"));
            m_shortcutManager->bind(actExit, PitchLabelerKeys::ShortcutExit, QStringLiteral("Exit"), QStringLiteral("File"));
            m_shortcutManager->bind(m_actUndo, PitchLabelerKeys::ShortcutUndo, QStringLiteral("Undo"), QStringLiteral("Edit"));
            m_shortcutManager->bind(m_actRedo, PitchLabelerKeys::ShortcutRedo, QStringLiteral("Redo"), QStringLiteral("Edit"));
            m_shortcutManager->bind(m_actZoomIn, PitchLabelerKeys::ShortcutZoomIn, QStringLiteral("Zoom In"), QStringLiteral("View"));
            m_shortcutManager->bind(m_actZoomOut, PitchLabelerKeys::ShortcutZoomOut, QStringLiteral("Zoom Out"), QStringLiteral("View"));
            m_shortcutManager->bind(m_actZoomReset, PitchLabelerKeys::ShortcutZoomReset, QStringLiteral("Reset Zoom"), QStringLiteral("View"));
            m_shortcutManager->bind(m_actABCompare, PitchLabelerKeys::ShortcutABCompare, QStringLiteral("A/B Compare"), QStringLiteral("Tools"));
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

            // Connect signals to update labels
            connect(this, &PitchLabelerPage::fileStatusChanged, statusFile, [statusFile](const QString &name) {
                statusFile->setText(name.isEmpty() ? QStringLiteral("未加载文件") : name);
            });
            connect(this, &PitchLabelerPage::positionChanged, statusPosition, [statusPosition](double sec) {
                int minutes = static_cast<int>(sec) / 60;
                double seconds = sec - minutes * 60;
                statusPosition->setText(
                    QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 6, 'f', 3, QChar('0')));
            });
            connect(this, &PitchLabelerPage::zoomChanged, statusZoom, [statusZoom](int percent) {
                statusZoom->setText(QString::number(percent) + "%");
            });
            connect(this, &PitchLabelerPage::noteCountChanged, statusNotes, [statusNotes](int count) {
                statusNotes->setText(QStringLiteral("音符数: ") + QString::number(count));
            });
            connect(this, &PitchLabelerPage::toolModeChanged, statusTool, [statusTool](int mode) {
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
