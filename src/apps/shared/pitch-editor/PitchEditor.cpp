#include "PitchEditor.h"

#include "DSFile.h"
#include "ui/PianoRollView.h"
#include "ui/PropertyPanel.h"
#include "ui/commands/NoteCommands.h"

#include <dstools/TimePos.h>

#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QProxyStyle>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

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
    : QWidget(parent),
      m_undoStack(new QUndoStack(this)),
      m_playWidget(new dstools::widgets::PlayWidget())
{
    m_viewport = new dstools::widgets::ViewportController(this);
    m_viewport->setPixelsPerSecond(100.0);

    buildActions();
    buildLayout();
    connectSignals();
}

PitchEditor::~PitchEditor() = default;

void PitchEditor::loadDSFile(std::shared_ptr<DSFile> file) {
    m_currentFile = std::move(file);
    m_undoStack->clear();
    m_mainStack->setCurrentIndex(1);
    m_pianoRoll->setDSFile(m_currentFile);
    m_propertyPanel->setDSFile(m_currentFile);
    m_actSave->setEnabled(true);

    if (m_playbackProgressSlider && m_currentFile) {
        double total = usToSec(m_currentFile->getTotalDuration());
        m_playbackProgressSlider->setRange(0, static_cast<int>(total * 1000));
        m_playbackProgressSlider->setValue(0);
        m_progressCurrentTime->setText("00:00.000");
        int totalMin = static_cast<int>(total) / 60;
        double totalSec = total - totalMin * 60;
        m_progressTotalTime->setText(QString("%1:%2").arg(totalMin, 2, 10, QChar('0')).arg(totalSec, 6, 'f', 3, QChar('0')));
    }

    updateStatusInfo();
}

void PitchEditor::loadAudio(const QString &path, double duration) {
    m_playWidget->openFile(path);
    if (m_pianoRoll && duration > 0) {
        m_pianoRoll->setAudioDuration(duration);
    }
}

void PitchEditor::clear() {
    m_currentFile.reset();
    m_mainStack->setCurrentIndex(0);
    m_pianoRoll->clear();
    m_propertyPanel->clear();
    updateStatusInfo();
}

void PitchEditor::setOriginalF0(const std::vector<int32_t> &f0) {
    Q_UNUSED(f0)
    // Stored externally by PitchLabelerPage for A/B comparison
}

void PitchEditor::setABComparisonActive(bool active) {
    m_abComparisonActive = active;
    m_pianoRoll->setABComparisonActive(active);
}

void PitchEditor::loadConfig(dstools::AppSettings &settings) {
    m_pianoRoll->loadConfig(settings);
}

void PitchEditor::pullConfig(dstools::AppSettings &settings) {
    m_pianoRoll->pullConfig(settings);
}

void PitchEditor::rebuildWindowShortcuts(dstools::AppSettings &settings) {
    qDeleteAll(m_windowShortcuts);
    m_windowShortcuts.clear();

    // Note: shortcut keys are bound by PitchLabelerPage through its ShortcutManager.
    // This method is reserved for future direct shortcut binding.
    Q_UNUSED(settings)
}

// ---- Actions ----

void PitchEditor::buildActions() {
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

    m_actZoomIn = new QAction(QStringLiteral("放大(&I)"), this);
    m_actZoomIn->setStatusTip(QStringLiteral("水平放大"));

    m_actZoomOut = new QAction(QStringLiteral("缩小(&O)"), this);
    m_actZoomOut->setStatusTip(QStringLiteral("水平缩小"));

    m_actZoomReset = new QAction(QStringLiteral("重置缩放(&R)"), this);
    m_actZoomReset->setStatusTip(QStringLiteral("重置缩放到默认级别"));

    m_actABCompare = new QAction(QStringLiteral("A/B 对比(&B)"), this);
    m_actABCompare->setCheckable(true);
    m_actABCompare->setStatusTip(QStringLiteral("切换 A/B 修改前后对比"));
}

void PitchEditor::buildLayout() {
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->setSpacing(0);

    // Vertical stack
    m_rightSplitter = new QSplitter(Qt::Vertical);
    pageLayout->addWidget(m_rightSplitter);

    // Stacked widget (empty state / content)
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

    // Page 1: Content
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

    auto *toolbarSpacer = new QWidget();
    toolbarSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(toolbarSpacer);

    m_actPlayPause = new QAction(this);
    m_actPlayPause->setIcon(QIcon(":/icons/play.svg"));
    m_actPlayPause->setStatusTip(QStringLiteral("播放/暂停 (Space)"));
    toolbar->addAction(m_actPlayPause);

    m_actStop = new QAction(this);
    m_actStop->setIcon(QIcon(":/icons/stop.svg"));
    m_actStop->setStatusTip(QStringLiteral("停止 (Escape)"));
    toolbar->addAction(m_actStop);

    toolbar->addSeparator();

    m_btnWaveformToggle = new QToolButton();
    m_btnWaveformToggle->setIcon(QIcon(":/icons/audio.svg"));
    m_btnWaveformToggle->setToolTip(QStringLiteral("切换波形显示"));
    m_btnWaveformToggle->setCheckable(true);
    m_btnWaveformToggle->setChecked(true);
    toolbar->addWidget(m_btnWaveformToggle);

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

    m_toolModeGroup = new QActionGroup(this);
    auto *actSelect = new QAction(QStringLiteral("选择"), m_toolModeGroup);
    actSelect->setCheckable(true);
    actSelect->setChecked(true);
    auto *actModulation = new QAction(QStringLiteral("颤音调制"), m_toolModeGroup);
    actModulation->setCheckable(true);
    auto *actDrift = new QAction(QStringLiteral("音高偏移"), m_toolModeGroup);
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

    contentLayout->addWidget(toolbar);

    // Playback progress
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
    m_playbackProgressSlider->setRange(0, 10000);
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

    // Piano roll
    m_pianoRoll = new ui::PianoRollView();
    m_pianoRoll->setViewportController(m_viewport);
    m_pianoRoll->setUndoStack(m_undoStack);
    contentLayout->addWidget(m_pianoRoll, 1);

    // Property panel
    m_propertyPanel = new ui::PropertyPanel();
    m_propertyPanel->setMinimumHeight(28);
    m_propertyPanel->setMaximumHeight(200);
    contentLayout->addWidget(m_propertyPanel);

    m_mainStack->addWidget(contentWidget);

    m_rightSplitter->addWidget(m_mainStack);

    m_rightSplitter->setStretchFactor(0, 1);
}

void PitchEditor::connectSignals() {
    // Edit actions
    connect(m_actUndo, &QAction::triggered, this, &PitchEditor::onUndo);
    connect(m_actRedo, &QAction::triggered, this, &PitchEditor::onRedo);

    // Undo stack
    connect(m_undoStack, &QUndoStack::canUndoChanged, m_actUndo, &QAction::setEnabled);
    connect(m_undoStack, &QUndoStack::canRedoChanged, m_actRedo, &QAction::setEnabled);

    // View actions
    connect(m_actZoomIn, &QAction::triggered, this, &PitchEditor::onZoomIn);
    connect(m_actZoomOut, &QAction::triggered, this, &PitchEditor::onZoomOut);
    connect(m_actZoomReset, &QAction::triggered, this, &PitchEditor::onZoomReset);

    // A/B compare
    connect(m_actABCompare, &QAction::toggled, this, [this](bool checked) {
        setABComparisonActive(checked);
    });

    // Piano roll signals
    connect(m_pianoRoll, &ui::PianoRollView::noteSelected, this, &PitchEditor::onNoteSelected);
    connect(m_pianoRoll, &ui::PianoRollView::positionClicked, this, &PitchEditor::onPositionClicked);
    connect(m_pianoRoll, &ui::PianoRollView::rulerClicked, this, [this](double timeSec) {
        if (m_playWidget) {
            m_playWidget->seek(timeSec);
            updatePlayheadPosition(timeSec);
        }
    });
    connect(m_pianoRoll, &ui::PianoRollView::fileEdited, this, [this]() {
        emit fileEdited();
        emit modificationChanged(true);
    });

    // Note editing from context menu — forward to page level
    connect(m_pianoRoll, &ui::PianoRollView::noteDeleteRequested, this, [this](const std::vector<int> &indices) {
        if (!m_currentFile || indices.empty()) return;
        m_undoStack->push(new ui::DeleteNotesCommand(m_currentFile, indices));
        m_pianoRoll->setDSFile(m_currentFile);
        emit fileEdited();
        emit modificationChanged(true);
    });
    connect(m_pianoRoll, &ui::PianoRollView::noteGlideChanged, this, [this](int idx, const QString &glide) {
        if (!m_currentFile || idx < 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
        QString oldGlide = m_currentFile->notes[idx].glide;
        m_undoStack->push(new ui::SetNoteGlideCommand(m_currentFile, idx, oldGlide, glide));
        m_pianoRoll->update();
        emit fileEdited();
        emit modificationChanged(true);
    });
    connect(m_pianoRoll, &ui::PianoRollView::noteSlurToggled, this, [this](int idx) {
        if (!m_currentFile || idx < 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
        int oldSlur = m_currentFile->notes[idx].slur;
        m_undoStack->push(new ui::ToggleNoteSlurCommand(m_currentFile, idx, oldSlur));
        m_pianoRoll->update();
        emit fileEdited();
        emit modificationChanged(true);
    });
    connect(m_pianoRoll, &ui::PianoRollView::noteRestToggled, this, [this](int idx) {
        if (!m_currentFile || idx < 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
        QString oldName = m_currentFile->notes[idx].name;
        bool wasRest = m_currentFile->notes[idx].isRest();
        m_undoStack->push(new ui::ToggleNoteRestCommand(m_currentFile, idx, oldName, wasRest));
        m_pianoRoll->update();
        emit fileEdited();
        emit modificationChanged(true);
    });
    connect(m_pianoRoll, &ui::PianoRollView::noteMergeLeft, this, [this](int idx) {
        if (!m_currentFile || idx <= 0 || idx >= static_cast<int>(m_currentFile->notes.size())) return;
        m_undoStack->push(new ui::MergeNoteLeftCommand(m_currentFile, idx));
        m_pianoRoll->setDSFile(m_currentFile);
        emit fileEdited();
        emit modificationChanged(true);
    });

    // Tool mode from piano roll
    connect(m_pianoRoll, &ui::PianoRollView::toolModeChanged, this, [this](int mode) {
        auto toolMode = static_cast<ui::ToolMode>(mode);
        m_btnToolSelect->setChecked(toolMode == ui::ToolSelect);
        m_btnToolModulation->setChecked(toolMode == ui::ToolModulation);
        m_btnToolDrift->setChecked(toolMode == ui::ToolDrift);
        emit toolModeChanged(mode);
    });

    // Playback
    connect(m_actPlayPause, &QAction::triggered, this, &PitchEditor::onPlayPause);
    connect(m_actStop, &QAction::triggered, this, &PitchEditor::onStop);
    connect(m_playWidget, &dstools::widgets::PlayWidget::playheadChanged, this, &PitchEditor::updatePlayheadPosition);

    // Seek via progress slider
    connect(m_playbackProgressSlider, &QSlider::valueChanged, this, [this](int value) {
        if (!m_playbackProgressSlider->isSliderDown()) return;
        if (!m_currentFile || !m_playWidget) return;
        double sec = value / 1000.0;
        m_playWidget->seek(sec);
        m_pianoRoll->setPlayheadTime(sec);
        updateTimeLabels(sec);
    });
}

// ---- Edit ----

void PitchEditor::onUndo() {
    m_undoStack->undo();
    m_pianoRoll->update();
    updateUndoRedoState();
    emit modificationChanged(true); // simplified — page checks actual state
}

void PitchEditor::onRedo() {
    m_undoStack->redo();
    m_pianoRoll->update();
    updateUndoRedoState();
    emit modificationChanged(true);
}

void PitchEditor::updateUndoRedoState() {
    m_actUndo->setEnabled(m_undoStack->canUndo());
    m_actRedo->setEnabled(m_undoStack->canRedo());
}

// ---- View ----

void PitchEditor::onZoomIn() {
    m_pianoRoll->zoomIn();
    updateZoomStatus();
}

void PitchEditor::onZoomOut() {
    m_pianoRoll->zoomOut();
    updateZoomStatus();
}

void PitchEditor::onZoomReset() {
    m_pianoRoll->resetZoom();
    updateZoomStatus();
}

void PitchEditor::updateZoomStatus() {
    int zoom = m_pianoRoll->getZoomPercent();
    emit zoomChanged(zoom);
}

// ---- Playback ----

void PitchEditor::onPlayPause() {
    m_playWidget->setPlaying(!m_playWidget->isPlaying());
}

void PitchEditor::onStop() {
    m_playWidget->setPlaying(false);
}

void PitchEditor::updateTimeLabels(double sec) {
    auto formatTime = [](double s) -> QString {
        int min = static_cast<int>(s) / 60;
        double sec = s - min * 60;
        return QString("%1:%2").arg(min, 2, 10, QChar('0')).arg(sec, 6, 'f', 3, QChar('0'));
    };

    m_progressCurrentTime->setText(formatTime(sec));
    if (m_currentFile) {
        m_progressTotalTime->setText(formatTime(usToSec(m_currentFile->getTotalDuration())));
    }

    emit positionChanged(sec);
}

void PitchEditor::updatePlayheadPosition(double sec) {
    m_pianoRoll->setPlayheadTime(sec);
    updateTimeLabels(sec);

    if (m_playbackProgressSlider && m_currentFile &&
        !m_playbackProgressSlider->isSliderDown()) {
        double total = usToSec(m_currentFile->getTotalDuration());
        m_playbackProgressSlider->setRange(0, static_cast<int>(total * 1000));
        m_playbackProgressSlider->setValue(static_cast<int>(sec * 1000));
    }
}

void PitchEditor::updatePlaybackState() {
    const bool playing = m_playWidget->isPlaying();
    m_actPlayPause->setIcon(QIcon(playing ? ":/icons/pause.svg" : ":/icons/play.svg"));
    m_actPlayPause->setToolTip(playing ? tr("Pause") : tr("Play"));
}

// ---- Tool mode ----

void PitchEditor::setToolMode(ui::ToolMode mode) {
    m_pianoRoll->setToolMode(mode);
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
