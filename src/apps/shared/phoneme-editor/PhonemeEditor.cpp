#include "PhonemeEditor.h"

#include "AudioVisualizerContainer.h"
#include "ui/SpectrogramColorPalette.h"

#include <QDataStream>
#include <QHBoxLayout>
#include <QIODevice>
#include <QIcon>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

#include <dstools/AudioDecoder.h>
#include <dstools/WaveFormat.h>

namespace dstools {
namespace phonemelabeler {

PhonemeEditor::PhonemeEditor(QWidget *parent)
    : QWidget(parent),
      m_document(new TextGridDocument(this)),
      m_undoStack(new QUndoStack(this)),
      m_playWidget(new dstools::widgets::PlayWidget()),
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
            m_container->fitToWindow();
        }

        m_waveformWidget->setAudioData(samples, sampleRate);
        m_powerWidget->setAudioData(samples, sampleRate);
        m_spectrogramWidget->setAudioData(samples, sampleRate);
        m_container->setAudioData(samples, sampleRate);
    }

    m_playWidget->openFile(audioPath);

    m_waveformWidget->setBoundaryModel(m_document);
    m_tierEditWidget->setDocument(m_document);
    m_powerWidget->setDocument(m_document);
    m_spectrogramWidget->setBoundaryModel(m_document);
    m_tierLabel->setBoundaryModel(m_document);
    m_entryListPanel->setDocument(m_document);

    emit documentLoaded();
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

void PhonemeEditor::setSpectrogramColorStyle(const QString &styleName) {
    m_spectrogramWidget->setColorPalette(SpectrogramColorPalette::fromName(styleName));
}

QByteArray PhonemeEditor::saveSplitterState() const {
    QByteArray result;
    QDataStream ds(&result, QIODevice::WriteOnly);
    ds << m_mainSplitter->saveState();
    ds << m_container->saveSplitterState();
    return result;
}

void PhonemeEditor::restoreSplitterState(const QByteArray &state) {
    if (state.isEmpty())
        return;
    QDataStream ds(state);
    QByteArray mainState, rightState;
    ds >> mainState >> rightState;
    if (!mainState.isEmpty())
        m_mainSplitter->restoreState(mainState);
    if (!rightState.isEmpty())
        m_container->restoreSplitterState(rightState);
}

void PhonemeEditor::saveViewportResolution() {
    m_container->saveResolution();
}

void PhonemeEditor::restoreViewportResolution() {
    m_container->restoreResolution();
}

void PhonemeEditor::saveChartVisibility() {
    m_container->saveChartVisibility();
}

void PhonemeEditor::restoreChartVisibility() {
    m_container->restoreChartVisibility();
    m_actTogglePower->setChecked(m_container->chartVisible(QStringLiteral("power")));
    m_actToggleSpectrogram->setChecked(m_container->chartVisible(QStringLiteral("spectrogram")));
}

QList<QAction *> PhonemeEditor::viewActions() const {
    return {m_actZoomIn, m_actZoomOut, m_actZoomReset, nullptr,
            m_actToggleBinding, nullptr,
            m_actTogglePower, m_actToggleSpectrogram};
}

// ---- Actions ----

void PhonemeEditor::buildActions() {
    m_actSave = new QAction(tr("&Save"), this);
    m_actSaveAs = new QAction(tr("Save &As..."), this);

    m_actUndo = new QAction(tr("&Undo"), this);
    connect(m_actUndo, &QAction::triggered, this, [this]() {
        if (m_undoStack->canUndo()) m_undoStack->undo();
    });

    m_actRedo = new QAction(tr("&Redo"), this);
    connect(m_actRedo, &QAction::triggered, this, [this]() {
        if (m_undoStack->canRedo()) m_undoStack->redo();
    });

    m_actZoomIn = new QAction(tr("Zoom &In"), this);
    connect(m_actZoomIn, &QAction::triggered, this, &PhonemeEditor::onZoomIn);

    m_actZoomOut = new QAction(tr("Zoom &Out"), this);
    connect(m_actZoomOut, &QAction::triggered, this, &PhonemeEditor::onZoomOut);

    m_actZoomReset = new QAction(tr("&Reset Zoom"), this);
    connect(m_actZoomReset, &QAction::triggered, this, &PhonemeEditor::onZoomReset);

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

    m_actToggleSpectrogram = new QAction(tr("Show &Spectrogram"), this);
    m_actToggleSpectrogram->setCheckable(true);
    m_actToggleSpectrogram->setChecked(true);
    connect(m_actToggleSpectrogram, &QAction::triggered, this, [this]() {
        setSpectrogramVisible(m_actToggleSpectrogram->isChecked());
        m_container->saveChartVisibility();
    });

    // Spectrogram color style submenu
    m_spectrogramColorMenu = new QMenu(tr("Spectrogram &Color"), this);
    m_spectrogramColorGroup = new QActionGroup(this);
    m_spectrogramColorGroup->setExclusive(true);
    for (const auto &palette : SpectrogramColorPalette::builtinPresets()) {
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

    m_actPlayPause = m_toolbar->addAction(tr("Play"), this, &PhonemeEditor::onPlayPause);
    m_actPlayPause->setIcon(QIcon(":/icons/play.svg"));
    m_actStop = m_toolbar->addAction(tr("Stop"), this, &PhonemeEditor::onStop);
    m_actStop->setIcon(QIcon(":/icons/stop.svg"));

    m_toolbar->addSeparator();

    // Tier selection combo
    m_tierCombo = new QComboBox(m_toolbar);
    m_tierCombo->setToolTip(tr("选择活跃层级"));
    m_tierCombo->setMinimumWidth(80);
    m_toolbar->addWidget(m_tierCombo);
    connect(m_tierCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        if (index >= 0 && m_document)
            m_document->setActiveTierIndex(index);
    });

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
        "QToolButton:checked {"
        "  background-color: #3d6fa5;"
        "  color: #ffffff;"
        "  border: 1px solid #5a9fd4;"
        "  border-radius: 3px;"
        "  padding: 3px 8px;"
        "}"
        "QToolButton:!checked {"
        "  background-color: transparent;"
        "  color: #888888;"
        "  border: 1px solid #555555;"
        "  border-radius: 3px;"
        "  padding: 3px 8px;"
        "}"
    );
    // Find the QToolButton widgets created by addAction and style them
    if (auto *btn = qobject_cast<QToolButton *>(m_toolbar->widgetForAction(m_actBindingToggle)))
        btn->setStyleSheet(kToggleStyle);
    if (auto *btn = qobject_cast<QToolButton *>(m_toolbar->widgetForAction(m_actSnapToggle)))
        btn->setStyleSheet(kToggleStyle);

    m_toolbar->addSeparator();
}

void PhonemeEditor::buildLayout() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(m_toolbar);

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainLayout->addWidget(m_mainSplitter);

    m_container = new AudioVisualizerContainer(QStringLiteral("PhonemeEditor"), this);
    m_container->setDefaultResolution(800); // ~20s visible at 44100Hz / 1200px viewport
    m_viewport = m_container->viewport();
    m_container->setPlayWidget(m_playWidget);

    m_tierLabel = new PhonemeTextGridTierLabel(m_container);
    m_tierLabel->setViewportController(m_viewport);
    m_container->setTierLabelArea(m_tierLabel);

    m_tierEditWidget = new TierEditWidget(m_document, m_undoStack, m_viewport, m_container->dragController(), m_container);
    m_container->setEditorWidget(m_tierEditWidget);

    m_waveformWidget = new WaveformWidget(m_viewport, m_container);
    m_waveformWidget->setUndoStack(m_undoStack);
    m_waveformWidget->setPlayWidget(m_playWidget);
    m_container->addChart(QStringLiteral("waveform"), m_waveformWidget, 1, 1, 2.0);

    m_powerWidget = new PowerWidget(m_viewport, m_container);
    m_powerWidget->setUndoStack(m_undoStack);
    m_powerWidget->setPlayWidget(m_playWidget);
    m_container->addChart(QStringLiteral("power"), m_powerWidget, 2, 1, 3.0);

    m_spectrogramWidget = new SpectrogramWidget(m_viewport, m_container);
    m_spectrogramWidget->setUndoStack(m_undoStack);
    m_spectrogramWidget->setPlayWidget(m_playWidget);
    m_container->addChart(QStringLiteral("spectrogram"), m_spectrogramWidget, 3, 1, 5.0);

    m_container->setBoundaryModel(m_document);

    auto *boundaryOverlay = m_container->boundaryOverlay();
    if (boundaryOverlay) {
        boundaryOverlay->setDocument(m_document);
        boundaryOverlay->setTierLabelGeometry(m_tierLabel->height(), m_tierLabel->tierRowHeight());
    }

    m_mainSplitter->addWidget(m_container);

    m_entryListPanel = new EntryListPanel(this);
    m_entryListPanel->setViewportController(m_viewport);
    m_entryListPanel->setMinimumWidth(160);
    m_entryListPanel->setMaximumWidth(300);
    m_mainSplitter->addWidget(m_entryListPanel);

    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 0);
}

void PhonemeEditor::connectSignals() {
    // Document signals
    connect(m_document, &TextGridDocument::documentChanged, this, [this]() {
        m_container->invalidateBoundaryModel();
        m_tierEditWidget->rebuildTierViews();
        m_entryListPanel->rebuildEntries();
        emit modificationChanged(m_document->isModified());

        // Rebuild tier combo when document structure changes
        m_tierCombo->blockSignals(true);
        m_tierCombo->clear();
        for (int t = 0; t < m_document->tierCount(); ++t) {
            QString name = m_document->tierName(t);
            if (name.isEmpty())
                name = QStringLiteral("Tier %1").arg(t + 1);
            m_tierCombo->addItem(name);
        }
        if (m_document->activeTierIndex() >= 0 && m_document->activeTierIndex() < m_tierCombo->count())
            m_tierCombo->setCurrentIndex(m_document->activeTierIndex());
        m_tierCombo->blockSignals(false);
    });

    // Undo stack
    connect(m_undoStack, &QUndoStack::canUndoChanged, m_actUndo, &QAction::setEnabled);
    connect(m_undoStack, &QUndoStack::canRedoChanged, m_actRedo, &QAction::setEnabled);
    connect(m_undoStack, &QUndoStack::undoTextChanged, [this](const QString &text) {
        m_actUndo->setText(tr("&Undo") + (text.isEmpty() ? QString() : (" (" + text + ")")));
    });
    connect(m_undoStack, &QUndoStack::redoTextChanged, [this](const QString &text) {
        m_actRedo->setText(tr("&Redo") + (text.isEmpty() ? QString() : (" (" + text + ")")));
    });

    // Viewport
    connect(m_viewport, &ViewportController::viewportChanged, m_waveformWidget, &WaveformWidget::setViewport);
    connect(m_viewport, &ViewportController::viewportChanged, m_tierEditWidget, &TierEditWidget::setViewport);
    connect(m_viewport, &ViewportController::viewportChanged, m_powerWidget, &PowerWidget::setViewport);
    connect(m_viewport, &ViewportController::viewportChanged, m_spectrogramWidget, &SpectrogramWidget::setViewport);

    auto *boundaryOverlay = m_container->boundaryOverlay();
    if (boundaryOverlay) {
        connect(m_viewport, &ViewportController::viewportChanged, boundaryOverlay, &BoundaryOverlayWidget::setViewport);
    }

    // Sync menu action checked state with widget visibility (for external visibility changes)
    connect(m_powerWidget, &PowerWidget::visibleStateChanged, this, [this](bool visible) {
        m_actTogglePower->setChecked(visible);
        emit powerVisibilityChanged(visible);
    });
    connect(m_spectrogramWidget, &SpectrogramWidget::visibleStateChanged, this, [this](bool visible) {
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
            });

    // Active tier -> repaint boundary overlays
    connect(m_document, &TextGridDocument::activeTierChanged, this, [this](int tier) {
        updateAllBoundaryOverlays();
        m_tierLabel->setActiveTierIndex(tier);
        if (auto *bo = m_container->boundaryOverlay()) {
            bo->setTierLabelGeometry(m_tierLabel->height(), m_tierLabel->tierRowHeight());
        }
    });
    connect(m_document, &TextGridDocument::boundaryMoved, this, [this](int, int, TimePos) {
        updateAllBoundaryOverlays();
        m_tierLabel->update();
        m_tierEditWidget->update();
        for (auto *child : m_tierEditWidget->findChildren<QWidget *>()) {
            child->update();
        }
        m_entryListPanel->rebuildEntries();
    });
    connect(m_document, &TextGridDocument::boundaryInserted, this, [this](int, int) {
        updateAllBoundaryOverlays();
        m_tierLabel->update();
    });
    connect(m_document, &TextGridDocument::boundaryRemoved, this, [this](int, int) {
        updateAllBoundaryOverlays();
        m_tierLabel->update();
    });

    // Tier label → document active tier sync
    connect(m_tierLabel, &PhonemeTextGridTierLabel::activeTierChanged, m_document, [this](int tier) {
        m_document->setActiveTierIndex(tier);
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
    connect(m_playWidget, &dstools::widgets::PlayWidget::playheadChanged,
            this, [this](double sec) {
                emit positionChanged(sec);
            });

    // Mouse wheel entry switching
    auto scrollEntry = [this](int delta) {
        int current = m_entryListPanel->currentEntryIndex();
        int newIndex = current + delta;
        if (newIndex < 0) newIndex = 0;
        m_entryListPanel->selectEntry(newIndex);
    };
    connect(m_waveformWidget, &WaveformWidget::entryScrollRequested, this, scrollEntry);
    connect(m_powerWidget, &PowerWidget::entryScrollRequested, this, scrollEntry);
    connect(m_spectrogramWidget, &SpectrogramWidget::entryScrollRequested, this, scrollEntry);

    auto *boundaryOverlay2 = m_container->boundaryOverlay();

    // Hover/drag state → boundary overlay
    connect(m_waveformWidget, &WaveformWidget::hoveredBoundaryChanged,
            boundaryOverlay2, &BoundaryOverlayWidget::setHoveredBoundary);
    connect(m_powerWidget, &PowerWidget::hoveredBoundaryChanged,
            boundaryOverlay2, &BoundaryOverlayWidget::setHoveredBoundary);
    connect(m_spectrogramWidget, &SpectrogramWidget::hoveredBoundaryChanged,
            boundaryOverlay2, &BoundaryOverlayWidget::setHoveredBoundary);

    // Drag started/finished → boundary overlay highlight
    auto *dragCtrl = m_container->dragController();
    connect(dragCtrl, &BoundaryDragController::dragStarted, this, [this](int, int boundaryIndex) {
        if (auto *bo = m_container->boundaryOverlay())
            bo->setDraggedBoundary(boundaryIndex);
    });
    connect(dragCtrl, &BoundaryDragController::dragFinished, this, [this](int, int, TimePos) {
        if (auto *bo = m_container->boundaryOverlay())
            bo->setDraggedBoundary(-1);
    });
}

void PhonemeEditor::updateAllBoundaryOverlays() {
    m_waveformWidget->updateBoundaryOverlay();
    m_powerWidget->updateBoundaryOverlay();
    m_spectrogramWidget->updateBoundaryOverlay();
    if (auto *bo = m_container->boundaryOverlay())
        bo->update();
}

void PhonemeEditor::onZoomIn() {
    m_viewport->zoomIn(m_viewport->viewCenter());
    emit zoomChanged(m_viewport->resolution());
}

void PhonemeEditor::onZoomOut() {
    m_viewport->zoomOut(m_viewport->viewCenter());
    emit zoomChanged(m_viewport->resolution());
}

void PhonemeEditor::onZoomReset() {
    m_viewport->setResolution(800); // default resolution for phoneme editing
    m_container->updateViewRangeFromResolution();
    emit zoomChanged(m_viewport->resolution());
}

void PhonemeEditor::onPlayPause() {
    m_playWidget->setPlaying(!m_playWidget->isPlaying());
}

void PhonemeEditor::onStop() {
    m_playWidget->setPlaying(false);
}

void PhonemeEditor::updatePlaybackState() {
    const bool playing = m_playWidget->isPlaying();
    if (playing) {
        m_actPlayPause->setIcon(QIcon(":/icons/pause.svg"));
        m_actPlayPause->setText(tr("Pause"));
    } else {
        m_actPlayPause->setIcon(QIcon(":/icons/play.svg"));
        m_actPlayPause->setText(tr("Play"));
    }
}

void PhonemeEditor::playBoundarySegment(double timeSec) {
    if (!m_document || !m_playWidget) return;

    int activeTier = m_document->activeTierIndex();
    if (activeTier < 0 || activeTier >= m_document->tierCount()) return;

    double segStart = 0.0;
    double segEnd = m_document->totalDuration();

    int count = m_document->boundaryCount(activeTier);
    for (int b = 0; b < count; ++b) {
        double t = m_document->boundaryTime(activeTier, b);
        if (t <= timeSec) segStart = t;
        if (t > timeSec) { segEnd = t; break; }
    }

    m_playWidget->setPlayRange(segStart, segEnd);
    m_playWidget->seek(segStart);
    m_playWidget->setPlaying(true);
}

} // namespace phonemelabeler
} // namespace dstools
