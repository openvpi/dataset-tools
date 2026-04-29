#include "PitchLabelerPage.h"

#include "DSFile.h"
#include "../PitchLabelerKeys.h"
#include "ui/FileListPanel.h"
#include "ui/PianoRollView.h"
#include "ui/PropertyPanel.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QProxyStyle>
#include <QShortcut>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <dstools/ShortcutManager.h>
#include <dstools/Theme.h>
#include <dstools/AudioFileResolver.h>

#include <algorithm>
#include <functional>

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

        PitchLabelerPage::PitchLabelerPage(QWidget *parent)
            : QWidget(parent), m_playWidget(new dstools::widgets::PlayWidget()),
              m_undoStack(new QUndoStack(this)) {

            m_viewport = new dstools::widgets::ViewportController(this);
            m_viewport->setPixelsPerSecond(100.0);

            buildActions();
            buildLayout();
            connectSignals();

            m_shortcutManager = new dstools::widgets::ShortcutManager(&m_settings, this);

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

            updateStatusInfo();
            emit workingDirectoryChanged(QString());
        }

        QList<QAction *> PitchLabelerPage::editActions() const {
            return {m_actUndo, m_actRedo, m_actSave, m_actSaveAll};
        }

        QList<QAction *> PitchLabelerPage::viewActions() const {
            return {m_actZoomIn, m_actZoomOut, m_actZoomReset};
        }

        QList<QAction *> PitchLabelerPage::toolActions() const {
            return {m_actABCompare};
        }

        bool PitchLabelerPage::hasUnsavedChanges() const {
            return m_currentFile && m_currentFile->modified;
        }

        bool PitchLabelerPage::save() {
            return saveFile();
        }

        bool PitchLabelerPage::saveAll() {
            return saveAllFiles();
        }

        void PitchLabelerPage::saveConfig() {
            if (!m_workingDirectory.isEmpty()) {
                m_settings.set(PitchLabelerKeys::LastDir, m_workingDirectory);
            }
            m_pianoRoll->pullConfig(m_settings);
            m_fileListPanel->saveState();
        }

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
            m_actPlayPause = new QAction(QStringLiteral("▶"), this);
            m_actPlayPause->setStatusTip(QStringLiteral("播放/暂停 (Space)"));
            toolbar->addAction(m_actPlayPause);

            m_actStop = new QAction(QStringLiteral("⏹"), this);
            m_actStop->setStatusTip(QStringLiteral("停止 (Escape)"));
            toolbar->addAction(m_actStop);

            toolbar->addSeparator();

            // ---- Waveform display toggle ----
            m_btnWaveformToggle = new QToolButton();
            m_btnWaveformToggle->setText(QStringLiteral("🔊"));
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
                    m_currentFile->markModified();
                    updateStatusInfo();
                    m_fileListPanel->setFileModified(m_currentFilePath, true);
                    emit modificationChanged(true);
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
                m_fileListPanel->setFileModified(m_currentFilePath, true);
                emit modificationChanged(true);
            });
            connect(m_pianoRoll, &ui::PianoRollView::noteGlideChanged, this, [this](int idx, const QString &glide) {
                if (!m_currentFile || idx < 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
                m_currentFile->notes[idx].glide = glide;
                m_currentFile->markModified();
                m_pianoRoll->update();
                m_fileListPanel->setFileModified(m_currentFilePath, true);
                emit modificationChanged(true);
            });
            connect(m_pianoRoll, &ui::PianoRollView::noteSlurToggled, this, [this](int idx) {
                if (!m_currentFile || idx < 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
                m_currentFile->notes[idx].slur = m_currentFile->notes[idx].slur ? 0 : 1;
                m_currentFile->markModified();
                m_pianoRoll->update();
                m_fileListPanel->setFileModified(m_currentFilePath, true);
                emit modificationChanged(true);
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
                m_fileListPanel->setFileModified(m_currentFilePath, true);
                emit modificationChanged(true);
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

        void PitchLabelerPage::applyConfig() {
            // Apply saved shortcuts
            m_shortcutManager->applyAll();

            // Window-level shortcuts for tool modes, playback, navigation
            rebuildWindowShortcuts();

            // Load piano roll display options
            m_pianoRoll->loadConfig(m_settings);

            // Restore last directory
            if (const QString savedDir = m_settings.get(PitchLabelerKeys::LastDir);
                !savedDir.isEmpty() && QDir(savedDir).exists()) {
                m_workingDirectory = savedDir;
                m_fileListPanel->setDirectory(savedDir);
            }
        }

        void PitchLabelerPage::loadFile(const QString &path) {
            auto [ds, error] = DSFile::load(path);
            if (!error.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("错误"),
                                     QStringLiteral("加载文件失败: ") + error);
                return;
            }

            m_currentFile = ds;
            m_currentFilePath = path;

            // Clear undo history for new file
            m_undoStack->clear();

            // Switch from empty state to file content
            m_mainStack->setCurrentIndex(1);

            // Update UI
            m_pianoRoll->setDSFile(ds);
            m_propertyPanel->setDSFile(ds);

            // Enable save
            m_actSave->setEnabled(true);

            // Find and load audio file
            QString audioPath = dstools::AudioFileResolver::findAudioFile(path);
            if (!audioPath.isEmpty()) {
                m_playWidget->openFile(audioPath);
            }

            // Limit piano roll length to audio file duration
            if (m_pianoRoll) {
                double audioDur = m_playWidget->duration();
                if (audioDur > 0)
                    m_pianoRoll->setAudioDuration(audioDur);
            }

            // Store original F0 for A/B comparison
            m_originalF0 = ds->f0.values;

            // Initialize progress bar
            if (m_playbackProgressSlider && ds) {
                double total = ds->getTotalDuration();
                m_playbackProgressSlider->setRange(0, static_cast<int>(total * 1000));
                m_playbackProgressSlider->setValue(0);
                m_progressCurrentTime->setText("00:00.000");
                int totalMin = static_cast<int>(total) / 60;
                double totalSec = total - totalMin * 60;
                m_progressTotalTime->setText(QString("%1:%2").arg(totalMin, 2, 10, QChar('0')).arg(totalSec, 6, 'f', 3, QChar('0')));
            }

            updateStatusInfo();

            emit fileLoaded(path);
            emit fileSelected(path);
        }

        bool PitchLabelerPage::saveFile() {
            if (!m_currentFile)
                return false;

            auto [ok, error] = m_currentFile->save();
            if (!ok) {
                QMessageBox::warning(this, QStringLiteral("错误"),
                                     QStringLiteral("保存失败: ") + error);
                return false;
            }

            updateStatusInfo();
            m_fileListPanel->setFileSaved(m_currentFilePath);
            emit fileSaved(m_currentFilePath);
            emit modificationChanged(false);
            return true;
        }

        bool PitchLabelerPage::saveAllFiles() {
            if (m_currentFile && m_currentFile->modified) {
                return saveFile();
            }
            return true;
        }

        void PitchLabelerPage::onUndo() {
            m_undoStack->undo();
            m_pianoRoll->update();
            updateUndoRedoState();
            emit modificationChanged(m_currentFile && m_currentFile->modified);
        }

        void PitchLabelerPage::onRedo() {
            m_undoStack->redo();
            m_pianoRoll->update();
            updateUndoRedoState();
            emit modificationChanged(m_currentFile && m_currentFile->modified);
        }

        void PitchLabelerPage::updateUndoRedoState() {
            m_actUndo->setEnabled(m_undoStack->canUndo());
            m_actRedo->setEnabled(m_undoStack->canRedo());
        }

        void PitchLabelerPage::onZoomIn() {
            m_pianoRoll->zoomIn();
            updateZoomStatus();
        }

        void PitchLabelerPage::onZoomOut() {
            m_pianoRoll->zoomOut();
            updateZoomStatus();
        }

        void PitchLabelerPage::onZoomReset() {
            m_pianoRoll->resetZoom();
            updateZoomStatus();
        }

        void PitchLabelerPage::updateZoomStatus() {
            int zoom = m_pianoRoll->getZoomPercent();
            emit zoomChanged(zoom);
        }

        void PitchLabelerPage::onPlayPause() {
            m_playWidget->setPlaying(!m_playWidget->isPlaying());
        }

        void PitchLabelerPage::onStop() {
            m_playWidget->setPlaying(false);
        }

        void PitchLabelerPage::updateTimeLabels(double sec) {
            auto formatTime = [](double s) -> QString {
                int min = static_cast<int>(s) / 60;
                double sec = s - min * 60;
                return QString("%1:%2").arg(min, 2, 10, QChar('0')).arg(sec, 6, 'f', 3, QChar('0'));
            };

            m_progressCurrentTime->setText(formatTime(sec));
            if (m_currentFile) {
                m_progressTotalTime->setText(formatTime(m_currentFile->getTotalDuration()));
            }

            emit positionChanged(sec);
        }

        void PitchLabelerPage::updatePlayheadPosition(double sec) {
            m_pianoRoll->setPlayheadTime(sec);
            updateTimeLabels(sec);

            // Don't fight the user — skip slider update while they're dragging
            if (m_playbackProgressSlider && m_currentFile &&
                !m_playbackProgressSlider->isSliderDown()) {
                double total = m_currentFile->getTotalDuration();
                m_playbackProgressSlider->setRange(0, static_cast<int>(total * 1000));
                m_playbackProgressSlider->setValue(static_cast<int>(sec * 1000));
            }
        }

        void PitchLabelerPage::updatePlaybackState() {
            const bool playing = m_playWidget->isPlaying();
            m_actPlayPause->setText(playing ? QStringLiteral("⏸") : QStringLiteral("▶"));
            m_actPlayPause->setToolTip(playing ? tr("Pause") : tr("Play"));
        }

        void PitchLabelerPage::setToolMode(ui::ToolMode mode) {
            m_pianoRoll->setToolMode(mode);
            m_btnToolSelect->setChecked(mode == ui::ToolSelect);
            m_btnToolModulation->setChecked(mode == ui::ToolModulation);
            m_btnToolDrift->setChecked(mode == ui::ToolDrift);
            emit toolModeChanged(static_cast<int>(mode));
        }

        void PitchLabelerPage::onNoteSelected(int noteIndex) {
            if (!m_currentFile)
                return;

            if (noteIndex >= 0 && noteIndex < static_cast<int>(m_currentFile->notes.size())) {
                m_propertyPanel->setSelectedNote(noteIndex);
            }
        }

        void PitchLabelerPage::onPositionClicked(double time, double midi) {
            Q_UNUSED(midi)
            emit positionChanged(time);
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

            if (m_currentFile) {
                emit noteCountChanged(m_currentFile->getNoteCount());
            } else {
                emit noteCountChanged(0);
            }

            emit modificationChanged(hasUnsavedChanges());
        }

        void PitchLabelerPage::reloadCurrentFile() {
            if (!m_currentFilePath.isEmpty()) {
                loadFile(m_currentFilePath);
            }
        }

        void PitchLabelerPage::rebuildWindowShortcuts() {
            // Delete previous shortcuts
            qDeleteAll(m_windowShortcuts);
            m_windowShortcuts.clear();

            struct Entry {
                const dstools::SettingsKey<QString> &key;
                std::function<void()> action;
            };

            const Entry entries[] = {
                {PitchLabelerKeys::ToolSelect, [this]() { setToolMode(ui::ToolSelect); }},
                {PitchLabelerKeys::ToolModulation, [this]() { setToolMode(ui::ToolModulation); }},
                {PitchLabelerKeys::ToolDrift, [this]() { setToolMode(ui::ToolDrift); }},
                {PitchLabelerKeys::PlaybackPlayPause, [this]() { onPlayPause(); }},
                {PitchLabelerKeys::PlaybackStop, [this]() { onStop(); }},
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

    } // namespace pitchlabeler
} // namespace dstools
