#include "PhonemeEditor.h"

#include "AudioVisualizerContainer.h"
#include "SpectrogramColorPalette.h"
#include "../pitch-editor/ui/PianoRollChartPanel.h"
#include "../pitch-editor/ui/PianoRollView.h"

#include <QHBoxLayout>
#include <QToolButton>
#include <QVBoxLayout>

#include <dsfw/Log.h>

#include <format>

namespace dstools {
namespace phonemelabeler {

PhonemeEditor::PhonemeEditor(QWidget *parent)
    : EditorContainerBase(QStringLiteral("PhonemeEditor"), parent),
      m_document(new TextGridDocument(this)),
      m_renderer(new WaveformRenderer(this))
{
    buildActions();
    buildToolbar();
    buildLayout();
    connectSignals();
}

// ---- Configuration ----

BoundaryDragController *PhonemeEditor::dragController() const {
    return m_container ? m_container->dragController() : nullptr;
}

void PhonemeEditor::setDocument(TextGridDocument *doc) {
    Q_UNUSED(doc)
    // The editor owns its own document. External callers interact via loadAudio
    // or future IEditorDataSource. This method is reserved for future use.
}

void PhonemeEditor::loadAudio(const QString &audioPath) {
    if (audioPath.isEmpty()) return;

    // Decode audio once via the renderer, then share the samples
    if (m_renderer->loadAudio(audioPath)) {
        const auto &samples = m_renderer->samples();
        int sampleRate = m_renderer->sampleRate();

        // Set viewport FIRST so that chart widgets receive correct PPS
        // when setAudioData triggers their initial rebuildMinMaxCache
        double audioDuration = sampleRate > 0 ? static_cast<double>(samples.size()) / sampleRate : 0.0;
        if (audioDuration > 0.0) {
            m_viewport->setAudioParams(sampleRate, static_cast<int64_t>(samples.size()));
            m_container->applyDefaultScale();
        }

        m_waveformChart->setAudioData(samples, sampleRate);
        m_powerChart->setAudioData(samples, sampleRate);
        m_spectrogramChart->setAudioData(samples, sampleRate);
        m_container->setAudioData(samples, sampleRate);
    }

    m_playWidget->openFile(audioPath);
    m_waveformChart->setBoundaryModel(m_document);
    m_tierEditWidget->setDocument(m_document);
    m_powerChart->setBoundaryModel(m_document);
    m_spectrogramChart->setBoundaryModel(m_document);
    m_entryListPanel->setDocument(m_document);

    emit documentLoaded();
}

void PhonemeEditor::loadDsPitchDocument(std::shared_ptr<pitchlabeler::DsPitchDocument> dsFile) {
    if (m_pianoRollChart) {
        m_pianoRollChart->pianoRollView()->setDsPitchDocument(std::move(dsFile));
    }
}

pitchlabeler::ui::PianoRollView *PhonemeEditor::pianoRollView() const {
    return m_pianoRollChart ? m_pianoRollChart->pianoRollView() : nullptr;
}

void PhonemeEditor::setBindingEnabled(bool enabled) {
    auto *ctrl = m_container->dragController();
    if (ctrl) ctrl->setBindingEnabled(enabled);
    m_actToggleBinding->setChecked(enabled);
    if (m_actBindingToggle) m_actBindingToggle->setChecked(enabled);
    emit bindingChanged(enabled);
}

void PhonemeEditor::setBindingToleranceMs(double ms) {
    auto *ctrl = m_container->dragController();
    if (ctrl) ctrl->setToleranceUs(msToUs(ms));
}

void PhonemeEditor::setSnapEnabled(bool enabled) {
    m_snapEnabled = enabled;
    auto *ctrl = m_container->dragController();
    if (ctrl) ctrl->setSnapEnabled(enabled);
    if (m_actSnapToggle)
        m_actSnapToggle->setChecked(enabled);
}

void PhonemeEditor::setPowerVisible(bool visible) {
    m_container->setChartVisible(QStringLiteral("power"), visible);
    m_actTogglePower->setChecked(visible);
}

void PhonemeEditor::setSpectrogramVisible(bool visible) {
    m_container->setChartVisible(QStringLiteral("spectrogram"), visible);
    m_actToggleSpectrogram->setChecked(visible);
}

void PhonemeEditor::setPianoRollVisible(bool visible) {
    m_container->setChartVisible(QStringLiteral("pianoroll"), visible);
    m_actTogglePianoRoll->setChecked(visible);
    updateBoundaryOverlayExclusions();
}

void PhonemeEditor::setSpectrogramColorStyle(const QString &styleName) {
    m_spectrogramChart->setColorPalette(dstools::SpectrogramColorPalette::fromName(styleName));
}

void PhonemeEditor::saveChartVisibility() {
    m_container->saveChartVisibility();
}

void PhonemeEditor::restoreChartVisibility() {
    m_container->restoreChartVisibility();
    m_actTogglePower->setChecked(m_container->chartVisible(QStringLiteral("power")));
    m_actToggleSpectrogram->setChecked(m_container->chartVisible(QStringLiteral("spectrogram")));
    m_actTogglePianoRoll->setChecked(m_container->chartVisible(QStringLiteral("pianoroll")));
}

QList<QAction *> PhonemeEditor::viewActions() const {
    return {m_actZoomIn, m_actZoomOut, m_actZoomReset, nullptr,
            m_actToggleBinding, nullptr,
            m_actTogglePower, m_actToggleSpectrogram, m_actTogglePianoRoll};
}

// ---- Actions ----

void PhonemeEditor::buildActions() {
    m_actSave = new QAction(tr("&Save"), this);
    m_actSaveAs = new QAction(tr("Save &As..."), this);

    m_actUndo->setText(tr("&Undo"));

    m_actRedo->setText(tr("&Redo"));

    m_actZoomIn = new QAction(tr("Zoom &In"), this);
    connect(m_actZoomIn, &QAction::triggered, this, &EditorContainerBase::onZoomIn);

    m_actZoomOut = new QAction(tr("Zoom &Out"), this);
    connect(m_actZoomOut, &QAction::triggered, this, &EditorContainerBase::onZoomOut);

    m_actZoomReset = new QAction(tr("&Reset Zoom"), this);
    connect(m_actZoomReset, &QAction::triggered, this, &EditorContainerBase::onZoomReset);

    m_actToggleBinding = new QAction(tr("&Boundary Binding"), this);
    m_actToggleBinding->setCheckable(true);
    m_actToggleBinding->setChecked(true); // default enabled
    connect(m_actToggleBinding, &QAction::triggered, this, [this]() {
        auto *ctrl = m_container->dragController();
        bool enabled = ctrl ? !ctrl->isBindingEnabled() : false;
        setBindingEnabled(enabled);
    });

    m_actTogglePower = new QAction(tr("Show &Power"), this);
    m_actTogglePower->setCheckable(true);
    m_actTogglePower->setChecked(true);
    connect(m_actTogglePower, &QAction::triggered, this, [this]() {
        setPowerVisible(m_actTogglePower->isChecked());
        m_container->saveChartVisibility();
    });

    m_actToggleSpectrogram = new QAction(tr("Spectrogram"), this);
    m_actToggleSpectrogram->setCheckable(true);
    m_actToggleSpectrogram->setChecked(true);

    connect(m_actToggleSpectrogram, &QAction::triggered, this, [this]() {
        setSpectrogramVisible(m_actToggleSpectrogram->isChecked());
        m_container->saveChartVisibility();
    });

    m_actTogglePianoRoll = new QAction(tr("Piano Roll"), this);
    m_actTogglePianoRoll->setCheckable(true);
    m_actTogglePianoRoll->setChecked(true);
    connect(m_actTogglePianoRoll, &QAction::triggered, this, [this]() {
        setPianoRollVisible(m_actTogglePianoRoll->isChecked());
        m_container->saveChartVisibility();
    });

    // Spectrogram color style submenu
    m_spectrogramColorMenu = new QMenu(tr("Spectrogram &Color"), this);
    m_spectrogramColorGroup = new QActionGroup(this);
    m_spectrogramColorGroup->setExclusive(true);
    for (const auto &palette : dstools::SpectrogramColorPalette::builtinPresets()) {
        QAction *act = m_spectrogramColorMenu->addAction(palette.name());
        act->setCheckable(true);
        m_spectrogramColorGroup->addAction(act);
        connect(act, &QAction::triggered, this, [this, name = palette.name()]() {
            setSpectrogramColorStyle(name);
        });
    }
}

void PhonemeEditor::buildToolbar() {
    m_toolbar = new QToolBar(tr("Main Toolbar"), this);
    m_toolbar->setMovable(false);

    createPlaybackActions();

    m_toolbar->addAction(playPauseAction());
    m_toolbar->addAction(stopAction());

    m_toolbar->addSeparator();

    m_actBindingToggle = m_toolbar->addAction(tr("Bind"), this, [this]() {
        auto *ctrl = m_container->dragController();
        bool enabled = ctrl ? !ctrl->isBindingEnabled() : false;
        setBindingEnabled(enabled);
    });
    m_actBindingToggle->setCheckable(true);
    m_actBindingToggle->setChecked(true);
    m_actBindingToggle->setToolTip(tr("绑定：拖动边界时同步其他层相同位置的边界"));

    m_actSnapToggle = m_toolbar->addAction(tr("Snap"), this, [this]() {
        setSnapEnabled(!m_snapEnabled);
    });
    m_actSnapToggle->setCheckable(true);
    m_actSnapToggle->setChecked(m_snapEnabled);
    m_actSnapToggle->setToolTip(tr("吸附：释放边界时自动对齐到附近其他层的边界"));

    // Apply distinct styling to Bind/Snap toggle buttons
    static const QString kToggleStyle = QStringLiteral(
        "QToolButton { padding: 3px 8px; border-radius: 3px; }"
        "QToolButton:hover { background-color: #2A2A38; color: #A0A0B0; }"
        "QToolButton:pressed { background-color: #1E1E2E; }"
        "QToolButton:checked {"
        "  background-color: #3d6fa5;"
        "  color: #ffffff;"
        "  border: 1px solid #5a9fd4;"
        "}"
        "QToolButton:!checked {"
        "  background-color: transparent;"
        "  color: #888888;"
        "  border: 1px solid #555555;"
        "}"
    );
    // Find the QToolButton widgets created by addAction and style them
    if (auto *btn = qobject_cast<QToolButton *>(m_toolbar->widgetForAction(m_actBindingToggle)))
        btn->setStyleSheet(kToggleStyle);
    if (auto *btn = qobject_cast<QToolButton *>(m_toolbar->widgetForAction(m_actSnapToggle)))
        btn->setStyleSheet(kToggleStyle);
}

void PhonemeEditor::buildLayout() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(m_toolbar);

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainLayout->addWidget(m_mainSplitter);

    setupSidebarToggle(m_mainSplitter, m_toolbar);

    setupContainerAndPlayWidget(800);
    m_container->setResolutionKey(QStringLiteral("Viewport/phoneme/resolution"));

    m_tierEditWidget = new TierEditWidget(m_document, m_undoStack, m_viewport, m_container->dragController(), m_container);
    m_container->setEditorWidget(m_tierEditWidget);
    m_tierEditWidget->setCoordConverter(&m_container->coordConverter());
    m_container->setTierRadioPanel(m_tierEditWidget->radioButtonContainer());

    addWaveformChart(1, 1, 2.0);
    m_waveformChart->setUndoStack(m_undoStack);

    addPowerChart(2, 1, 3.0);
    m_powerChart->setUndoStack(m_undoStack);

    addMouthCurveChart(2, 1, 0.5);

    addSpectrogramChart(3, 1, 5.0);
    m_spectrogramChart->setUndoStack(m_undoStack);

    // Add piano roll chart
    pitchlabeler::ui::PianoRollChartPanel::registerChartConfig();
    m_pianoRollChart = new pitchlabeler::ui::PianoRollChartPanel(m_viewport);
    m_pianoRollChart->setUndoStack(m_undoStack);
    m_container->addChart("pianoroll", m_pianoRollChart, 0, 0, 3.0);

    m_container->restoreChartVisibility();

    m_container->setBoundaryModel(m_document);

    m_waveformChart->setBoundaryModel(m_document);
    m_powerChart->setBoundaryModel(m_document);
    m_spectrogramChart->setBoundaryModel(m_document);

    m_mainSplitter->addWidget(m_container);

    m_container->setupPhonemeOverlay(m_document->tierCount(), 24);

    m_container->setUndoStack(m_undoStack);

    // 配置边界拖动控制器的吸附和绑定功能
    if (auto *dragController = m_container->dragController()) {
        dragController->setSnapEnabled(true);
        dragController->setBindingEnabled(true);
        dragController->setPixelSnapThreshold(10.0); // 10像素吸附阈值
        dragController->setToleranceUs(1000); // 1ms绑定容差
    }

    m_entryListPanel = new EntryListPanel(this);
    m_entryListPanel->setViewportController(m_viewport);
    m_entryListPanel->setMinimumWidth(160);
    m_entryListPanel->setMaximumWidth(300);
    m_mainSplitter->addWidget(m_entryListPanel);

    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 0);
    m_mainSplitter->setCollapsible(1, true);
    m_sidebarExpandedSize = m_mainSplitter->sizes();

    updateBoundaryOverlayExclusions();
}

void PhonemeEditor::connectSignals() {
    // Document signals
    connect(m_document, &TextGridDocument::documentChanged, this, [this]() {
        m_container->invalidateBoundaryModel();
        m_tierEditWidget->rebuildTierViews();
        m_entryListPanel->rebuildEntries();
        emit modificationChanged(m_document->isModified());

        // 层级结构变化时，TierEditWidget会自动更新单选按钮
    });

    // Undo stack
    connectUndoRedo();
    connect(m_undoStack, &QUndoStack::undoTextChanged, [this](const QString &text) {
        m_actUndo->setText(tr("&Undo") + (text.isEmpty() ? QString() : (" (" + text + ")")));
    });
    connect(m_undoStack, &QUndoStack::redoTextChanged, [this](const QString &text) {
        m_actRedo->setText(tr("&Redo") + (text.isEmpty() ? QString() : (" (" + text + ")")));
    });

    connect(this, &EditorContainerBase::zoomChanged, this, [this](int) {
        m_actZoomIn->setEnabled(m_viewport->canZoomIn());
        m_actZoomOut->setEnabled(m_viewport->canZoomOut());
    });

    // Viewport connections are managed by AudioVisualizerContainer via
    // connectViewportToWidget (called by addChart/setEditorWidget) and
    // the constructor (TimeRuler, BoundaryOverlay, MiniMapScrollBar).
    // Direct connections here would cause duplicate setViewport() calls.

    // Sync menu action checked state with widget visibility (for external visibility changes)
    connect(m_powerChart, &dstools::PowerChartPanel::visibleStateChanged, this, [this](bool visible) {
        m_actTogglePower->setChecked(visible);
        emit powerVisibilityChanged(visible);
    });
    connect(m_spectrogramChart, &dstools::SpectrogramChartPanel::visibleStateChanged, this, [this](bool visible) {
        m_actToggleSpectrogram->setChecked(visible);
        emit spectrogramVisibilityChanged(visible);
    });

    // Sync toggle actions when chart visibility changes via container API
    connect(m_container, &AudioVisualizerContainer::chartVisibilityChanged, this,
            [this](const QString &id, bool visible) {
                if (id == QStringLiteral("power"))
                    m_actTogglePower->setChecked(visible);
                else if (id == QStringLiteral("spectrogram"))
                    m_actToggleSpectrogram->setChecked(visible);
                else if (id == QStringLiteral("pianoroll")) {
                    m_actTogglePianoRoll->setChecked(visible);
                    emit pianoRollVisibilityChanged(visible);
                }
            });

    // Keep zoom button enabled states in sync (Ctrl+wheel bypasses menu actions)
    connect(m_viewport, &dsfw::widgets::ViewportController::viewportChanged,
            this, [this](const dsfw::widgets::ViewportState &) {
                m_actZoomIn->setEnabled(m_viewport->canZoomIn());
                m_actZoomOut->setEnabled(m_viewport->canZoomOut());
            });

    // Active tier -> repaint boundary overlays
    connect(m_document, &TextGridDocument::activeTierChanged, this, [this](int tier) {
        Q_UNUSED(tier)
        updateAllBoundaryOverlays();
        if (auto *bo = m_container->boundaryOverlay()) {
            bo->setTierLabelGeometry(m_document->tierCount() * 24, 24);
        }
    });
    connect(m_document, &TextGridDocument::boundaryMoved, this, [this](int, int, TimePos) {
        updateAllBoundaryOverlays();
        m_tierEditWidget->update();
        for (auto *child : m_tierEditWidget->findChildren<QWidget *>()) {
            child->update();
        }
    });
    connect(m_document, &TextGridDocument::boundaryInserted, this, [this](int, int) {
        updateAllBoundaryOverlays();
    });
    connect(m_document, &TextGridDocument::boundaryRemoved, this, [this](int, int) {
        updateAllBoundaryOverlays();
    });

    // Entry list panel: rebuild when active tier changes or boundaries change
    connect(m_document, &TextGridDocument::activeTierChanged, this, [this](int) {
        m_entryListPanel->rebuildEntries();
    });
    connect(m_document, &TextGridDocument::boundaryInserted, this, [this](int, int) {
        m_entryListPanel->rebuildEntries();
    });
    connect(m_document, &TextGridDocument::boundaryRemoved, this, [this](int, int) {
        m_entryListPanel->rebuildEntries();
    });
    connect(m_document, &TextGridDocument::intervalTextChanged, this, [this](int, int) {
        m_entryListPanel->rebuildEntries();
    });

    // Playback: positionChanged signal (playhead itself handled by container)
    connect(m_playWidget, &dsfw::widgets::PlayWidget::playheadChanged,
            this, [this](double sec) {
                emit positionChanged(sec);
            });

    // Playback error relay to parent page (for status bar / toast notification)
    connect(m_playWidget, &dsfw::widgets::PlayWidget::playbackError,
            this, [this](const QString &msg) {
                DSFW_LOG_WARN("audio", ("Playback error: " + msg).toStdString().c_str());
                emit fileStatusChanged(msg);
            });

    // Tier view right-click playback
    connect(m_tierEditWidget, &TierEditWidget::requestPlayback, this,
            [this](TimePos startTime, TimePos endTime) {
                if (!m_playWidget) return;
                // Guard against zero-length intervals (startTime == endTime in TextGrid)
                if (startTime >= endTime) {
                    // Extend zero-length intervals by 50ms for audible playback
                    endTime = startTime + 50000;
                    DSFW_LOG_DEBUG("PhonemeEditor",
                                   std::format("Extending zero-length interval playback to 50ms (start={})",
                                               usToSec(startTime)));
                }
                double startSec = usToSec(startTime);
                double endSec = usToSec(endTime);
                m_playWidget->setPlaying(false);
                if (startSec >= endSec) {
                    DSFW_LOG_WARN("PhonemeEditor",
                                  std::format("Cannot play: invalid range [{}, {}]", startSec, endSec));
                    return;
                }
                m_playWidget->setPlayRange(startSec, endSec);
                m_playWidget->seek(startSec);
                m_playWidget->setPlaying(true);
            });

    // Mouse wheel entry switching
    auto scrollEntry = [this](int delta) {
        int current = m_entryListPanel->currentEntryIndex();
        int newIndex = current + delta;
        if (newIndex < 0) newIndex = 0;
        m_entryListPanel->selectEntry(newIndex);
    };
    connect(m_waveformChart, &dstools::WaveformChartPanel::entryScrollRequested, this, scrollEntry);
    connect(m_powerChart, &dstools::PowerChartPanel::entryScrollRequested, this, scrollEntry);
    connect(m_spectrogramChart, &dstools::SpectrogramChartPanel::entryScrollRequested, this, scrollEntry);

    auto *boundaryOverlay2 = m_container->boundaryOverlay();

    // Hover/drag state -> boundary overlay
    connect(m_waveformChart, &dstools::WaveformChartPanel::hoveredBoundaryChanged,
            boundaryOverlay2, &BoundaryOverlayWidget::setHoveredBoundary);
    connect(m_powerChart, &dstools::PowerChartPanel::hoveredBoundaryChanged,
            boundaryOverlay2, &BoundaryOverlayWidget::setHoveredBoundary);
    connect(m_spectrogramChart, &dstools::SpectrogramChartPanel::hoveredBoundaryChanged,
            boundaryOverlay2, &BoundaryOverlayWidget::setHoveredBoundary);

    connect(m_tierEditWidget, &TierEditWidget::tierHoveredBoundaryChanged, this,
            [this](int tierIndex, int boundaryIndex) {
                if (auto *bo = m_container->boundaryOverlay()) {
                    int activeTier = m_document->activeTierIndex();
                    bo->setHoveredBoundary(tierIndex == activeTier ? boundaryIndex : -1);
                }
            });

    // Drag started/finished → boundary overlay highlight + real-time UI refresh
    auto *dragCtrl = m_container->dragController();
    connect(dragCtrl, &BoundaryDragController::dragStarted, this, [this](int, int boundaryIndex) {
        if (auto *bo = m_container->boundaryOverlay())
            bo->setDraggedBoundary(boundaryIndex);
    });
    connect(dragCtrl, &BoundaryDragController::dragging, this,
            [this](int, int, TimePos) {
                m_container->invalidateBoundaryModel();
                m_tierEditWidget->update();
                for (auto *child : m_tierEditWidget->findChildren<QWidget *>()) {
                    child->update();
                }
            });
    connect(dragCtrl, &BoundaryDragController::dragFinished, this, [this](int, int, TimePos) {
        if (auto *bo = m_container->boundaryOverlay())
            bo->setDraggedBoundary(-1);
        m_entryListPanel->rebuildEntries();
    });
}

void PhonemeEditor::updateAllBoundaryOverlays() {
    m_container->invalidateBoundaryModel();
}

void PhonemeEditor::updateBoundaryOverlayExclusions() {
    auto *bo = m_container->boundaryOverlay();
    if (!bo) return;

    if (!m_pianoRollChart || m_pianoRollChart->isHidden()) {
        bo->setExcludedYRanges({});
        return;
    }

    auto *splitter = m_container->chartSplitter();
    QPoint topLeft = m_pianoRollChart->mapTo(splitter, QPoint(0, 0));
    QRect pianoRect = m_pianoRollChart->geometry();
    bo->setExcludedYRanges(
        {{topLeft.y(), topLeft.y() + pianoRect.height()}});
}

void PhonemeEditor::playBoundarySegment(double timeSec) {
    if (!m_document || !m_playWidget) return;

    int activeTier = m_document->activeTierIndex();
    if (activeTier < 0 || activeTier >= m_document->tierCount()) return;

    double segStart = 0.0;
    double segEnd = usToSec(m_document->totalDuration());

    int count = m_document->boundaryCount(activeTier);
    for (int b = 0; b < count; ++b) {
        double t = usToSec(m_document->boundaryTime(activeTier, b));
        if (t <= timeSec) segStart = t;
        if (t > timeSec) { segEnd = t; break; }
    }

    // Validate segment bounds before playback
    if (segStart >= segEnd) {
        DSFW_LOG_WARN("PhonemeEditor", "Invalid segment range at timeSec");
        return;
    }

    m_playWidget->setPlayRange(segStart, segEnd);
    m_playWidget->seek(segStart);
    m_playWidget->setPlaying(true);
}

} // namespace phonemelabeler
} // namespace dstools
