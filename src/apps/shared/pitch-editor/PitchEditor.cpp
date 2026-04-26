#include "PitchEditor.h"

#include <dstools/DsPitchDocument.h>
#include "ui/PianoRollChartPanel.h"
#include "ui/PropertyPanel.h"
#include "ui/commands/NoteCommands.h"

#include <dstools/TimePos.h>
#include "../audio-visualizer/AudioVisualizerContainer.h"
#include "../audio-visualizer/dstools/MiniMapScrollBar.h"
#include <WaveformChartPanel.h>

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QProxyStyle>
#include <QSlider>
#include <QActionGroup>
#include <QSplitter>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QDataStream>

#include <functional>

namespace {
    /// Style override: left-click anywhere on the slider groove jumps directly
    /// to that position (instead of paging).
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

PitchEditor::PitchEditor(QWidget *parent)
    : EditorContainerBase("pitch-editor", parent)
{
    // Setup container and play widget with default resolution (440 = equivalent to setPixelsPerSecond(100))
    setupContainerAndPlayWidget(440);
    m_container->setResolutionKey("Viewport/pitch/resolution");
    
    buildActions();
    buildLayout();
    connectSignals();
}

PitchEditor::~PitchEditor() = default;

void PitchEditor::loadDsPitchDocument(std::shared_ptr<DsPitchDocument> file) {
    m_currentFile = std::move(file);
    m_undoStack->clear();
    m_mainStack->setCurrentIndex(1);
    
    // Set DsPitchDocument to all components
    if (m_pianoRollChart) {
        m_pianoRollChart->setDsPitchDocument(m_currentFile);
        m_pianoRollChart->setUndoStack(m_undoStack);
    }
    m_propertyPanel->setDsPitchDocument(m_currentFile);
    m_actSave->setEnabled(true);

    // Set total duration to container
    if (m_currentFile) {
        double total = usToSec(m_currentFile->getTotalDuration());
        if (total > 0.0) {
            m_container->setTotalDuration(total);
        }
    }

    updateStatusInfo();
}

void PitchEditor::loadAudio(const QString &path, double duration) {
    m_playWidget->openFile(path);
    if (duration <= 0.0 && m_currentFile) {
        duration = usToSec(m_currentFile->getTotalDuration());
    }
    if (duration <= 0.0) {
        duration = m_playWidget->duration();
    }
    if (duration > 0) {
        m_container->setTotalDuration(duration);
        // Playback progress is now handled by the container's PlayCursorOverlay
        // m_playbackProgressSlider, m_progressCurrentTime, and m_progressTotalTime are removed
        if (m_container && m_container->miniMap()) {
            m_container->miniMap()->setTotalDuration(duration);
        }
    }
}

void PitchEditor::clear() {
    m_currentFile.reset();
    m_mainStack->setCurrentIndex(0);
    if (m_pianoRollChart) {
        m_pianoRollChart->setDsPitchDocument(nullptr);
    }
    m_propertyPanel->clear();
    updateStatusInfo();
}

void PitchEditor::setABComparisonActive(bool active) {
    m_abComparisonActive = active;
    if (m_pianoRollChart && m_pianoRollChart->pianoRollView()) {
        m_pianoRollChart->pianoRollView()->setABComparisonActive(active);
    }
}

void PitchEditor::loadConfig(dstools::AppSettings &settings) {
    if (m_pianoRollChart && m_pianoRollChart->pianoRollView()) {
        m_pianoRollChart->pianoRollView()->loadConfig(settings);
    }
}

void PitchEditor::pullConfig(dstools::AppSettings &settings) {
    if (m_pianoRollChart && m_pianoRollChart->pianoRollView()) {
        m_pianoRollChart->pianoRollView()->pullConfig(settings);
    }
}

// ---- Actions ----

void PitchEditor::buildActions() {
    createPlaybackActions();

    m_actSave = new QAction(tr("Save(&S)"), this);
    m_actSave->setStatusTip(tr("Save current file"));
    m_actSave->setEnabled(false);

    m_actSaveAll = new QAction(tr("Save All(&L)"), this);
    m_actSaveAll->setStatusTip(tr("Save all modified files"));

    m_actUndo->setText(tr("Undo(&U)"));
    m_actUndo->setStatusTip(tr("Undo last operation"));
    m_actUndo->setEnabled(false);

    m_actRedo->setText(tr("Redo(&R)"));
    m_actRedo->setStatusTip(tr("Redo last undone operation"));
    m_actRedo->setEnabled(false);

    m_actZoomIn = new QAction(tr("Zoom In(&I)"), this);
    m_actZoomIn->setStatusTip(tr("Zoom in horizontally"));

    m_actZoomOut = new QAction(tr("Zoom Out(&O)"), this);
    m_actZoomOut->setStatusTip(tr("Zoom out horizontally"));

    m_actZoomReset = new QAction(tr("Reset Zoom(&R)"), this);
    m_actZoomReset->setStatusTip(tr("Reset zoom to default level"));

    m_actABCompare = new QAction(tr("A/B Compare(&B)"), this);
    m_actABCompare->setCheckable(true);
    m_actABCompare->setStatusTip(tr("Toggle A/B before-after comparison"));
}

void PitchEditor::buildLayout() {
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    // Stacked widget (empty state / content)
    m_mainStack = new QStackedWidget();

    // Page 0: Empty state
    m_emptyPage = new QWidget();
    auto *emptyLayout = new QVBoxLayout(m_emptyPage);
    emptyLayout->setAlignment(Qt::AlignCenter);
    auto *emptyLabel = new QLabel(tr("Open a file first"));
    emptyLabel->setAlignment(Qt::AlignCenter);
    emptyLabel->setStyleSheet("color: #6F737A; font-size: 14px; padding: 40px;");
    emptyLayout->addWidget(emptyLabel);
    m_mainStack->addWidget(m_emptyPage);

    // Page 1: Content
    auto *contentWidget = new QWidget();
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    // Toolbar
    m_toolbar = new QToolBar();
    m_toolbar->setMovable(false);
    m_toolbar->setFloatable(false);
    m_toolbar->setIconSize(QSize(18, 18));
    m_toolbar->setStyleSheet(
        "QToolBar { background: #2A2A36; border-bottom: 1px solid #33333E; spacing: 4px; padding: 2px 6px; }"
        "QToolBar QToolButton { padding: 4px 12px; border-radius: 3px; font-size: 12px; color: #9898A8; }"
        "QToolBar QToolButton:hover { background-color: #3A3A48; color: #C0C0D0; }"
        "QToolBar QToolButton:pressed { background-color: #2A2A3A; color: #D0D0E0; }"
        "QToolBar QToolButton:checked { background-color: #4A90D9; color: white; }");

    static const QString toolBtnStyle = QStringLiteral(
        "QToolButton { padding: 4px 12px; border-radius: 3px; font-size: 12px; }"
        "QToolButton:hover { background-color: #3A3A48; color: #C0C0D0; }"
        "QToolButton:pressed { background-color: #2A2A3A; color: #D0D0E0; }"
        "QToolButton:checked { background-color: #4A90D9; color: white; }"
        "QToolButton:!checked { background: transparent; color: #9898A8; }");

    m_btnToolSelect = new QToolButton();
    m_btnToolSelect->setText(tr("↑ Select"));
    m_btnToolSelect->setToolTip(tr("Select Tool (V)"));
    m_btnToolSelect->setCheckable(true);
    m_btnToolSelect->setChecked(true);
    m_btnToolSelect->setStyleSheet(toolBtnStyle);
    m_toolbar->addWidget(m_btnToolSelect);

    m_btnToolModulation = new QToolButton();
    m_btnToolModulation->setText(tr("≡ Modulation"));
    m_btnToolModulation->setToolTip(tr("Modulation Tool (M)"));
    m_btnToolModulation->setCheckable(true);
    m_btnToolModulation->setStyleSheet(toolBtnStyle);
    m_toolbar->addWidget(m_btnToolModulation);

    m_btnToolDrift = new QToolButton();
    m_btnToolDrift->setText(tr("↕ Pitch Drift"));
    m_btnToolDrift->setToolTip(tr("Pitch Drift Tool (D)"));
    m_btnToolDrift->setCheckable(true);
    m_btnToolDrift->setStyleSheet(toolBtnStyle);
    m_toolbar->addWidget(m_btnToolDrift);

    m_btnToolAudition = new QToolButton();
    m_btnToolAudition->setText(tr("✎ Audition"));
    m_btnToolAudition->setToolTip(tr("Audition Tool"));
    m_btnToolAudition->setCheckable(true);
    m_btnToolAudition->setStyleSheet(toolBtnStyle);
    m_toolbar->addWidget(m_btnToolAudition);

    auto *toolbarSpacer = new QWidget();
    toolbarSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolbar->addWidget(toolbarSpacer);

    m_toolbar->addAction(playPauseAction());
    m_toolbar->addAction(stopAction());

    m_toolbar->addSeparator();

    // 根据用户要求，pitchlabel不需要显示波形图，移除波形图切换按钮
    // m_btnWaveformToggle = new QToolButton();
    // m_btnWaveformToggle->setIcon(QIcon(":/icons/audio.svg"));
    // m_btnWaveformToggle->setToolTip(tr("Toggle waveform display"));
    // m_btnWaveformToggle->setCheckable(true);
    // m_btnWaveformToggle->setChecked(true);
    // m_toolbar->addWidget(m_btnWaveformToggle);
    // connect(m_btnWaveformToggle, &QToolButton::toggled, this, [this](bool checked) {
    //     // Waveform toggle now controls waveform chart visibility in container
    //     if (m_container) {
    //         m_container->setChartVisible("waveform", checked);
    //     }
    // });

    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setFixedWidth(80);
    m_volumeSlider->setStyleSheet(
        "QSlider { margin: 0 4px; }"
        "QSlider::groove:horizontal { background: #33333E; height: 4px; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #4A90D9; width: 10px; margin: -3px 0; border-radius: 5px; }");
    m_toolbar->addWidget(m_volumeSlider);

    m_volumeLabel = new QLabel(QStringLiteral("100%"));
    m_volumeLabel->setStyleSheet("color: #9898A8; font-size: 11px; margin-left: 2px;");
    m_toolbar->addWidget(m_volumeLabel);

    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        m_volumeLabel->setText(QString::number(value) + QStringLiteral("%"));
    });

    m_toolModeGroup = new QActionGroup(this);
    auto *actSelect = new QAction(tr("Select"), m_toolModeGroup);
    actSelect->setCheckable(true);
    actSelect->setChecked(true);
    auto *actModulation = new QAction(tr("Modulation"), m_toolModeGroup);
    actModulation->setCheckable(true);
    auto *actDrift = new QAction(tr("Pitch Drift"), m_toolModeGroup);
    actDrift->setCheckable(true);
    m_toolModeGroup->setExclusive(true);

    connect(m_btnToolSelect, &QToolButton::clicked, this, [this]() {
        setToolMode(ui::ToolSelect);
    });
    connect(m_btnToolModulation, &QToolButton::clicked, this, [this]() {
        setToolMode(ui::ToolModulation);
    });
    connect(m_btnToolDrift, &QToolButton::clicked, this, [this]() {
        setToolMode(ui::ToolDrift);
    });

    contentLayout->addWidget(m_toolbar);

    // === 主水平分割器（左：钢琴窗容器，右：属性面板） ===
    m_mainSplitter = new QSplitter(Qt::Horizontal);
    contentLayout->addWidget(m_mainSplitter, 1);
    m_container->setTierLabelArea(nullptr);

    m_pianoRollChart = new ui::PianoRollChartPanel(m_viewport);
    m_pianoRollChart->setUndoStack(m_undoStack);
    m_container->addChart("pianoroll", m_pianoRollChart, 0, 0, 3.0);

    // === 波形图表面板 ===
    // 根据用户要求，pitchlabel不需要显示波形图
    // addWaveformChart(1, 1, 1.0);
    // m_waveformChart->setUndoStack(m_undoStack);

    // 容器放入上方（钢琴窗）
    m_mainSplitter->addWidget(m_container);

    // === PropertyPanel (右侧属性面板) ===
    m_propertyPanel = new ui::PropertyPanel();
    m_propertyPanel->setMinimumWidth(250);
    m_propertyPanel->setMaximumWidth(400);
    m_mainSplitter->addWidget(m_propertyPanel);

    m_mainSplitter->setStretchFactor(0, 3);  // 钢琴窗占3份
    m_mainSplitter->setStretchFactor(1, 1);  // 属性面板占1份
    m_mainSplitter->setCollapsible(1, true);

    setupSidebarToggle(m_mainSplitter, m_toolbar);

    m_mainStack->addWidget(contentWidget);
    m_mainStack->setCurrentIndex(0);  // 初始空状态

    pageLayout->addWidget(m_mainStack);
}

void PitchEditor::connectSignals() {
    connectUndoRedo();

    // Undo stack

    // View actions
    connect(m_actZoomIn, &QAction::triggered, this, &PitchEditor::onZoomIn);
    connect(m_actZoomOut, &QAction::triggered, this, &PitchEditor::onZoomOut);
    connect(m_actZoomReset, &QAction::triggered, this, &PitchEditor::onZoomReset);

    connect(m_viewport, &dsfw::widgets::ViewportController::viewportChanged,
            this, [this](const dsfw::widgets::ViewportState &) {
                if (m_pianoRollChart && m_pianoRollChart->pianoRollView()) {
                    int z = m_pianoRollChart->pianoRollView()->getZoomPercent();
                    m_actZoomIn->setEnabled(z < 5000);
                    m_actZoomOut->setEnabled(z > 2);
                }
            });

    // A/B compare
    connect(m_actABCompare, &QAction::toggled, this, [this](bool checked) {
        setABComparisonActive(checked);
    });

    // Piano roll chart signals
    if (m_pianoRollChart) {
        connect(m_pianoRollChart, &ui::PianoRollChartPanel::noteSelected, this, &PitchEditor::onNoteSelected);
        connect(m_pianoRollChart, &ui::PianoRollChartPanel::positionClicked, this, &PitchEditor::onPositionClicked);
        connect(m_pianoRollChart, &ui::PianoRollChartPanel::rulerClicked, this, [this](double timeSec) {
            if (m_playWidget) {
                m_playWidget->seek(timeSec);
                updatePlayheadPosition(timeSec);
            }
        });
        connect(m_pianoRollChart, &ui::PianoRollChartPanel::fileEdited, this, [this]() {
            emit fileEdited();
            emit modificationChanged(true);
        });

        // Note editing from context menu — forward to page level
        connect(m_pianoRollChart, &ui::PianoRollChartPanel::noteDeleteRequested, this, [this](const std::vector<int> &indices) {
            if (!m_currentFile || indices.empty()) return;
            m_undoStack->push(new ui::DeleteNotesCommand(m_currentFile, indices));
            if (m_pianoRollChart) {
                m_pianoRollChart->setDsPitchDocument(m_currentFile);
            }
            emit fileEdited();
            emit modificationChanged(true);
        });
        connect(m_pianoRollChart, &ui::PianoRollChartPanel::noteGlideChanged, this, [this](int idx, const QString &glide) {
            if (!m_currentFile || idx < 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
            QString oldGlide = m_currentFile->notes[idx].glide;
            m_undoStack->push(new ui::SetNoteGlideCommand(m_currentFile, idx, oldGlide, glide));
            if (m_pianoRollChart) {
                m_pianoRollChart->update();
            }
            emit fileEdited();
            emit modificationChanged(true);
        });
        connect(m_pianoRollChart, &ui::PianoRollChartPanel::noteSlurToggled, this, [this](int idx) {
            if (!m_currentFile || idx < 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
            int oldSlur = m_currentFile->notes[idx].slur;
            m_undoStack->push(new ui::ToggleNoteSlurCommand(m_currentFile, idx, oldSlur));
            if (m_pianoRollChart) {
                m_pianoRollChart->update();
            }
            emit fileEdited();
            emit modificationChanged(true);
        });
        connect(m_pianoRollChart, &ui::PianoRollChartPanel::noteRestToggled, this, [this](int idx) {
            if (!m_currentFile || idx < 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
            QString oldName = m_currentFile->notes[idx].name;
            bool wasRest = m_currentFile->notes[idx].isRest();
            m_undoStack->push(new ui::ToggleNoteRestCommand(m_currentFile, idx, oldName, wasRest));
            if (m_pianoRollChart) {
                m_pianoRollChart->update();
            }
            emit fileEdited();
            emit modificationChanged(true);
        });
        connect(m_pianoRollChart, &ui::PianoRollChartPanel::noteMergeLeft, this, [this](int idx) {
            if (!m_currentFile || idx <= 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
            m_undoStack->push(new ui::MergeNoteLeftCommand(m_currentFile, idx));
            if (m_pianoRollChart) {
                m_pianoRollChart->setDsPitchDocument(m_currentFile);
            }
            emit fileEdited();
            emit modificationChanged(true);
        });

        // Tool mode from piano roll
        connect(m_pianoRollChart, &ui::PianoRollChartPanel::toolModeChanged, this, [this](int mode) {
            auto toolMode = static_cast<ui::ToolMode>(mode);
            m_btnToolSelect->setChecked(toolMode == ui::ToolSelect);
            m_btnToolModulation->setChecked(toolMode == ui::ToolModulation);
            m_btnToolDrift->setChecked(toolMode == ui::ToolDrift);
            emit toolModeChanged(mode);
        });
    }

    // Playback
    connect(m_playWidget, &dsfw::widgets::PlayWidget::playheadChanged, this, &PitchEditor::updatePlayheadPosition);

    // Seek via container's MiniMapScrollBar (handled by container)
    // No need for separate progress slider connections
}

// ---- Edit ----

void PitchEditor::onUndo() {
    AudioEditorWidgetBase::onUndo();
    if (m_pianoRollChart) {
        m_pianoRollChart->update();
    }
}

void PitchEditor::onRedo() {
    AudioEditorWidgetBase::onRedo();
    if (m_pianoRollChart) {
        m_pianoRollChart->update();
    }
}

// ---- View ----

void PitchEditor::onZoomIn() {
    EditorContainerBase::onZoomIn();
    updateZoomStatus();
}

void PitchEditor::onZoomOut() {
    EditorContainerBase::onZoomOut();
    updateZoomStatus();
}

void PitchEditor::onZoomReset() {
    EditorContainerBase::onZoomReset();
    updateZoomStatus();
}

void PitchEditor::updateZoomStatus() {
    // Zoom status is handled by EditorContainerBase
    emit zoomChanged(m_viewport->resolution());
}

// ---- Playback ----

void PitchEditor::updateTimeLabels(double sec) {
    // Time labels are now handled by container's TimeRulerWidget
    // This method is kept for compatibility but does nothing
    Q_UNUSED(sec)
    emit positionChanged(sec);
}

void PitchEditor::updatePlayheadPosition(double sec) {
    if (m_pianoRollChart && m_pianoRollChart->pianoRollView()) {
        m_pianoRollChart->pianoRollView()->setPlayheadTime(sec);
    }
    // Time labels are now handled by container's TimeRulerWidget
    // No need for separate time label updates
}

void PitchEditor::setToolMode(ui::ToolMode mode) {
    if (m_pianoRollChart && m_pianoRollChart->pianoRollView()) {
        m_pianoRollChart->pianoRollView()->setToolMode(mode);
    }
    m_btnToolSelect->setChecked(mode == ui::ToolSelect);
    m_btnToolModulation->setChecked(mode == ui::ToolModulation);
    m_btnToolDrift->setChecked(mode == ui::ToolDrift);
    emit toolModeChanged(static_cast<int>(mode));
}

// ---- Selection ----

void PitchEditor::onNoteSelected(int noteIndex) {
    if (!m_currentFile) return;
    if (noteIndex >= 0 && noteIndex < static_cast<int>(m_currentFile->notes.size())) {
        m_propertyPanel->setSelectedNote(noteIndex);
    }
    emit noteSelected(noteIndex);
}

void PitchEditor::onPositionClicked(double time, double midi) {
    Q_UNUSED(midi)
    emit positionChanged(time);
}

// ---- Status ----

void PitchEditor::updateStatusInfo() {
    if (m_currentFile) {
        emit noteCountChanged(m_currentFile->getNoteCount());
    } else {
        emit noteCountChanged(0);
    }
}

} // namespace pitchlabeler
} // namespace dstools
