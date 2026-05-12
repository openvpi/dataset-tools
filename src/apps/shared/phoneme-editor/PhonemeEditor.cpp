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
    : ChartPageBase(QStringLiteral("PhonemeEditor"), parent),
      m_document(new TextGridDocument(this)),
      m_undoStack(new QUndoStack(this)),
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
    m_spectrogramChart->setColorPalette(SpectrogramColorPalette::fromName(styleName));
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
    connect(m_actZoomIn, &QAction::triggered, this, &ChartPageBase::onZoomIn);

    m_actZoomOut = new QAction(tr("Zoom &Out"), this);
    connect(m_actZoomOut, &QAction::triggered, this, &ChartPageBase::onZoomOut);

    m_actZoomReset = new QAction(tr("&Reset Zoom"), this);
    connect(m_actZoomReset, &QAction::triggered, this, &ChartPageBase::onZoomReset);

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

    setupContainerAndPlayWidget(800);

    m_tierEditWidget = new TierEditWidget(m_document, m_undoStack, m_viewport, m_container->dragController(), m_container);
    m_container->setEditorWidget(m_tierEditWidget);

    addWaveformChart(1, 1, 2.0);
    m_waveformChart->setUndoStack(m_undoStack);

    addPowerChart(2, 1, 3.0);
    m_powerChart->setUndoStack(m_undoStack);

    addSpectrogramChart(3, 1, 5.0);
    m_spectrogramChart->setUndoStack(m_undoStack);

    m_container->setBoundaryModel(m_document);

    m_tierLabelPanel = new TierLabelPanel(m_viewport, m_container);
    m_tierLabelPanel->setBoundaryModel(m_document);
    {
        std::vector<TierLabelRow> rows;
        for (int t = 0; t < m_document->tierCount(); ++t) {
            TierLabelRow row;
            row.tierIndex = t;
            row.labelY = 20.0 + t * 28.0;
            row.rowHeight = 24.0;
            row.textColor = (t == m_document->activeTierIndex())
                ? QColor(255, 220, 100) : QColor(180, 180, 200);
            row.bgColor = QColor(40, 40, 45, 200);
            QFont f = font();
            f.setPixelSize(12);
            row.font = f;
            rows.push_back(row);
        }
        m_tierLabelPanel->setRows(rows);
    }
    m_tierLabelPanel->setViewportState(m_viewport->state());
    m_tierLabelPanel->setPixelWidth(m_container->width());
    m_tierLabelPanel->hide();

    m_waveformChart->setBoundaryModel(m_document);
    m_powerChart->setBoundaryModel(m_document);
    m_spectrogramChart->setBoundaryModel(m_document);

    auto *boundaryOverlay = m_container->boundaryOverlay();
    if (boundaryOverlay) {
        boundaryOverlay->setDocument(m_document);
        boundaryOverlay->setTierLabelGeometry(0, 24);
    }

    m_mainSplitter->addWidget(m_container);

    m_entryListPanel = new EntryListPanel(this);
    m_entryListPanel->setViewportController(m_viewport);
    m_entryListPanel->setMinimumWidth(160);
    m_entryListPanel->setMaximumWidth(300);
    m_mainSplitter->addWidget(m_entryListPanel);

    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 0);

    int editorHeight = m_document->tierCount() * 24;
    if (boundaryOverlay) {
        boundaryOverlay->setExtraTopOffset(editorHeight);
    }

    m_container->removeTierLabelArea();

    if (auto *bo = m_container->boundaryOverlay())
        bo->forceReposition();
}

void PhonemeEditor::connectSignals() {
    // Document signals
    connect(m_document, &TextGridDocument::documentChanged, this, [this]() {
        m_container->invalidateBoundaryModel();
        m_tierEditWidget->rebuildTierViews();
        m_entryListPanel->rebuildEntries();
        if (m_tierLabelPanel) {
            std::vector<TierLabelRow> rows;
            for (int t = 0; t < m_document->tierCount(); ++t) {
                TierLabelRow row;
                row.tierIndex = t;
                row.labelY = 20.0 + t * 28.0;
                row.rowHeight = 24.0;
                row.textColor = (t == m_document->activeTierIndex())
                    ? QColor(255, 220, 100) : QColor(180, 180, 200);
                row.bgColor = QColor(40, 40, 45, 200);
                QFont f = m_tierLabelPanel->font();
                f.setPixelSize(12);
                row.font = f;
                rows.push_back(row);
            }
            m_tierLabelPanel->setRows(rows);
        }
        emit modificationChanged(m_document->isModified());

        // Rebuild tier combo when document structure changes
        m_tierCombo->blockSignals(true);
        m_tierCombo->clear();
        for (int t = 0; t < m_document->tierCount(); ++t) {
            QString name = m_document->tierName(t);
            if (name.isEmpty())
                name = QStringLiteral("Tier %1").arg(t + 1);
            if (name == QStringLiteral("raw_text"))
                continue;
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

    connect(this, &ChartPageBase::zoomChanged, this, [this](int) {
        m_actZoomIn->setEnabled(m_viewport->canZoomIn());
        m_actZoomOut->setEnabled(m_viewport->canZoomOut());
    });

    // Viewport connections are managed by AudioVisualizerContainer via
    // connectViewportToWidget (called by addChart/setEditorWidget) and
    // the constructor (TimeRuler, BoundaryOverlay, MiniMapScrollBar).
    // Direct connections here would cause duplicate setViewport() calls.

    // Sync menu action checked state with widget visibility (for external visibility changes)
    connect(m_powerChart, &PowerChartPanel::visibleStateChanged, this, [this](bool visible) {
        m_actTogglePower->setChecked(visible);
        emit powerVisibilityChanged(visible);
    });
    connect(m_spectrogramChart, &SpectrogramChartPanel::visibleStateChanged, this, [this](bool visible) {
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

    // Keep zoom button enabled states in sync (Ctrl+wheel bypasses menu actions)
    connect(m_viewport, &dstools::widgets::ViewportController::viewportChanged,
            this, [this](const dstools::widgets::ViewportState &) {
                m_actZoomIn->setEnabled(m_viewport->canZoomIn());
                m_actZoomOut->setEnabled(m_viewport->canZoomOut());
            });

    // Active tier -> repaint boundary overlays
    connect(m_document, &TextGridDocument::activeTierChanged, this, [this](int tier) {
        updateAllBoundaryOverlays();
        if (auto *bo = m_container->boundaryOverlay()) {
            bo->setTierLabelGeometry(0, 24);
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
    connect(m_waveformChart, &WaveformChartPanel::entryScrollRequested, this, scrollEntry);
    connect(m_powerChart, &PowerChartPanel::entryScrollRequested, this, scrollEntry);
    connect(m_spectrogramChart, &SpectrogramChartPanel::entryScrollRequested, this, scrollEntry);

    auto *boundaryOverlay2 = m_container->boundaryOverlay();

    // Hover/drag state -> boundary overlay
    connect(m_waveformChart, &WaveformChartPanel::hoveredBoundaryChanged,
            boundaryOverlay2, &BoundaryOverlayWidget::setHoveredBoundary);
    connect(m_powerChart, &PowerChartPanel::hoveredBoundaryChanged,
            boundaryOverlay2, &BoundaryOverlayWidget::setHoveredBoundary);
    connect(m_spectrogramChart, &SpectrogramChartPanel::hoveredBoundaryChanged,
            boundaryOverlay2, &BoundaryOverlayWidget::setHoveredBoundary);

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
