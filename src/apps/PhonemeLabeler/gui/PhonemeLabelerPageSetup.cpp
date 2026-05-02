#include "PhonemeLabelerPage.h"
#include "ui/WaveformWidget.h"
#include "ui/WaveformRenderer.h"
#include "ui/TierEditWidget.h"
#include "ui/PowerWidget.h"
#include "ui/SpectrogramWidget.h"
#include "ui/SpectrogramColorPalette.h"
#include "ui/FileListPanel.h"
#include "ui/EntryListPanel.h"
#include "ui/TimeRulerWidget.h"
#include "ui/TextGridDocument.h"
#include <dstools/ViewportController.h>
#include "ui/BoundaryBindingManager.h"
#include "ui/BoundaryOverlayWidget.h"

#include <QScrollBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>

namespace dstools {
namespace phonemelabeler {

void PhonemeLabelerPage::buildActions() {
    // Edit actions
    m_actSave = new QAction(tr("&Save"), this);
    connect(m_actSave, &QAction::triggered, this, &PhonemeLabelerPage::saveFile);

    m_actSaveAs = new QAction(tr("Save &As..."), this);
    connect(m_actSaveAs, &QAction::triggered, this, &PhonemeLabelerPage::saveFileAs);

    m_actUndo = new QAction(tr("&Undo"), this);
    connect(m_actUndo, &QAction::triggered, this, [this]() {
        if (m_undoStack->canUndo()) m_undoStack->undo();
    });

    m_actRedo = new QAction(tr("&Redo"), this);
    connect(m_actRedo, &QAction::triggered, this, [this]() {
        if (m_undoStack->canRedo()) m_undoStack->redo();
    });

    // View actions
    m_actZoomIn = new QAction(tr("Zoom &In"), this);
    connect(m_actZoomIn, &QAction::triggered, this, &PhonemeLabelerPage::onZoomIn);

    m_actZoomOut = new QAction(tr("Zoom &Out"), this);
    connect(m_actZoomOut, &QAction::triggered, this, &PhonemeLabelerPage::onZoomOut);

    m_actZoomReset = new QAction(tr("&Reset Zoom"), this);
    connect(m_actZoomReset, &QAction::triggered, this, &PhonemeLabelerPage::onZoomReset);

    m_actToggleBinding = new QAction(tr("&Boundary Binding"), this);
    m_actToggleBinding->setCheckable(true);
    connect(m_actToggleBinding, &QAction::triggered, this, [this]() {
        bool enabled = !m_bindingManager->isEnabled();
        m_bindingManager->setEnabled(enabled);
        m_settings.set(PhonemeLabelerKeys::BoundaryBindingEnabled, enabled);
        emit bindingChanged(enabled);
        m_actToggleBinding->setChecked(enabled);
        if (m_actBindingToggle) m_actBindingToggle->setChecked(enabled);
    });

    m_actTogglePower = new QAction(tr("Show &Power"), this);
    m_actTogglePower->setCheckable(true);
    connect(m_actTogglePower, &QAction::triggered, this, [this]() {
        bool enabled = m_actTogglePower->isChecked();
        m_powerWidget->setVisible(enabled);
        m_settings.set(PhonemeLabelerKeys::PowerEnabled, enabled);
    });

    m_actToggleSpectrogram = new QAction(tr("Show &Spectrogram"), this);
    m_actToggleSpectrogram->setCheckable(true);
    connect(m_actToggleSpectrogram, &QAction::triggered, this, [this]() {
        bool enabled = m_actToggleSpectrogram->isChecked();
        m_spectrogramWidget->setVisible(enabled);
        m_settings.set(PhonemeLabelerKeys::SpectrogramEnabled, enabled);
    });

    // Spectrogram color style submenu
    m_spectrogramColorMenu = new QMenu(tr("Spectrogram &Color"), this);
    m_spectrogramColorGroup = new QActionGroup(this);
    m_spectrogramColorGroup->setExclusive(true);
    QString currentStyle = m_settings.get(PhonemeLabelerKeys::SpectrogramColorStyle);
    for (const auto &palette : SpectrogramColorPalette::builtinPresets()) {
        QAction *act = m_spectrogramColorMenu->addAction(palette.name());
        act->setCheckable(true);
        act->setChecked(palette.name().compare(currentStyle, Qt::CaseInsensitive) == 0);
        m_spectrogramColorGroup->addAction(act);
        connect(act, &QAction::triggered, this, [this, name = palette.name()]() {
            m_spectrogramWidget->setColorPalette(SpectrogramColorPalette::fromName(name));
            m_settings.set(PhonemeLabelerKeys::SpectrogramColorStyle, name);
        });
    }
}

void PhonemeLabelerPage::buildToolbar() {
    m_toolbar = new QToolBar(tr("Main Toolbar"), this);
    m_toolbar->setMovable(false);

    m_actPlayPause = m_toolbar->addAction(tr("Play"), this, &PhonemeLabelerPage::onPlayPause);
    m_actStop = m_toolbar->addAction(tr("Stop"), this, &PhonemeLabelerPage::onStop);

    m_toolbar->addSeparator();

    m_actBindingToggle = m_toolbar->addAction(tr("Binding"), this, [this]() {
        bool enabled = !m_bindingManager->isEnabled();
        m_bindingManager->setEnabled(enabled);
        m_settings.set(PhonemeLabelerKeys::BoundaryBindingEnabled, enabled);
        emit bindingChanged(enabled);
        m_actToggleBinding->setChecked(enabled);
        m_actBindingToggle->setChecked(enabled);
    });
    m_actBindingToggle->setCheckable(true);

    m_toolbar->addSeparator();
    m_toolbar->addWidget(m_playWidget);
}

void PhonemeLabelerPage::buildLayout() {
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainLayout->addWidget(m_mainSplitter);

    // Left: file list panel
    m_fileListPanel = new FileListPanel(this);
    m_mainSplitter->addWidget(m_fileListPanel);

    // Center: container with time ruler + vertical splitter + horizontal scrollbar
    auto *centerContainer = new QWidget(this);
    auto *centerLayout = new QVBoxLayout(centerContainer);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);

    // Time ruler at top
    m_timeRulerWidget = new TimeRulerWidget(m_viewport, this);
    centerLayout->addWidget(m_timeRulerWidget);

    // Vertical splitter with tiers + waveform + power + spectrogram
    m_rightSplitter = new QSplitter(Qt::Vertical, this);
    centerLayout->addWidget(m_rightSplitter, 1);

    // Tier editor
    m_tierEditWidget = new TierEditWidget(m_document, m_undoStack, m_viewport, m_bindingManager, this);
    m_rightSplitter->addWidget(m_tierEditWidget);

    // Waveform
    m_waveformWidget = new WaveformWidget(m_viewport, this);
    m_waveformWidget->setBindingManager(m_bindingManager);
    m_waveformWidget->setUndoStack(m_undoStack);
    m_waveformWidget->setPlayWidget(m_playWidget);
    m_rightSplitter->addWidget(m_waveformWidget);

    // Power
    m_powerWidget = new PowerWidget(m_viewport, this);
    m_powerWidget->setBindingManager(m_bindingManager);
    m_powerWidget->setUndoStack(m_undoStack);
    m_rightSplitter->addWidget(m_powerWidget);

    // Spectrogram
    m_spectrogramWidget = new SpectrogramWidget(m_viewport, this);
    m_spectrogramWidget->setBindingManager(m_bindingManager);
    m_spectrogramWidget->setUndoStack(m_undoStack);
    m_rightSplitter->addWidget(m_spectrogramWidget);

    m_rightSplitter->setStretchFactor(0, 0); // tier: fixed
    m_rightSplitter->setStretchFactor(1, 1); // waveform: 1/4
    m_rightSplitter->setStretchFactor(2, 1); // power: 1/4
    m_rightSplitter->setStretchFactor(3, 2); // spectrogram: 1/2
    m_rightSplitter->setHandleWidth(1);

    // Boundary overlay
    m_boundaryOverlay = new BoundaryOverlayWidget(m_viewport, centerContainer);
    m_boundaryOverlay->trackWidget(m_rightSplitter);

    // Horizontal scrollbar at bottom
    m_hScrollBar = new QScrollBar(Qt::Horizontal, this);
    m_hScrollBar->setMinimum(0);
    m_hScrollBar->setMaximum(0);
    centerLayout->addWidget(m_hScrollBar);

    m_mainSplitter->addWidget(centerContainer);

    // Right: entry list panel
    m_entryListPanel = new EntryListPanel(this);
    m_entryListPanel->setViewportController(m_viewport);
    m_entryListPanel->setMinimumWidth(160);
    m_entryListPanel->setMaximumWidth(300);
    m_mainSplitter->addWidget(m_entryListPanel);

    // Set initial split ratios
    m_mainSplitter->setStretchFactor(0, 0); // file list: fixed
    m_mainSplitter->setStretchFactor(1, 1); // center: stretch
    m_mainSplitter->setStretchFactor(2, 0); // entry list: fixed
}

void PhonemeLabelerPage::connectSignals() {
    // Document signals
    connect(m_document, &TextGridDocument::documentChanged, this, [this]() {
        emit modificationChanged(m_document->isModified());
        if (m_currentFilePath.isEmpty()) {
            emit fileStatusChanged(tr("No file"));
        } else {
            emit fileStatusChanged(QFileInfo(m_currentFilePath).fileName());
        }
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
    connect(m_viewport, &ViewportController::viewportChanged, m_timeRulerWidget, &TimeRulerWidget::setViewport);
    connect(m_viewport, &ViewportController::viewportChanged, m_boundaryOverlay, &BoundaryOverlayWidget::setViewport);

    // Viewport → scrollbar sync
    connect(m_viewport, &ViewportController::viewportChanged, this, [this](const ViewportState &) {
        updateScrollBar();
    });

    // Scrollbar → viewport
    connect(m_hScrollBar, &QScrollBar::valueChanged, this, &PhonemeLabelerPage::onScrollBarValueChanged);

    // Active tier → repaint boundary overlays on all chart widgets
    connect(m_document, &TextGridDocument::activeTierChanged, this, [this](int) {
        updateAllBoundaryOverlays();
    });
    // Also repaint overlays when boundaries move (real-time during drag)
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

    // File list
    connect(m_fileListPanel, &FileListPanel::fileSelected, this, &PhonemeLabelerPage::onFileSelected);

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

    // Playback
    connect(m_playWidget, &dstools::widgets::PlayWidget::playheadChanged,
            [this](double sec) {
                m_waveformWidget->setPlayhead(sec);
                emit positionChanged(sec);
            });

    // Mouse wheel entry switching (plain wheel → next/previous entry)
    auto scrollEntry = [this](int delta) {
        int current = m_entryListPanel->currentEntryIndex();
        int newIndex = current + delta;
        if (newIndex < 0) newIndex = 0;
        m_entryListPanel->selectEntry(newIndex);
    };
    connect(m_waveformWidget, &WaveformWidget::entryScrollRequested, this, scrollEntry);
    connect(m_powerWidget, &PowerWidget::entryScrollRequested, this, scrollEntry);
    connect(m_spectrogramWidget, &SpectrogramWidget::entryScrollRequested, this, scrollEntry);
    connect(m_timeRulerWidget, &TimeRulerWidget::entryScrollRequested, this, scrollEntry);

    // Hover/drag state → boundary overlay for continuous lines
    connect(m_waveformWidget, &WaveformWidget::hoveredBoundaryChanged,
            m_boundaryOverlay, &BoundaryOverlayWidget::setHoveredBoundary);
    connect(m_powerWidget, &PowerWidget::hoveredBoundaryChanged,
            m_boundaryOverlay, &BoundaryOverlayWidget::setHoveredBoundary);
    connect(m_spectrogramWidget, &SpectrogramWidget::hoveredBoundaryChanged,
            m_boundaryOverlay, &BoundaryOverlayWidget::setHoveredBoundary);

    // Drag started → overlay shows drag highlight
    auto onDragStarted = [this](int /*tierIndex*/, int boundaryIndex) {
        m_boundaryOverlay->setDraggedBoundary(boundaryIndex);
    };
    connect(m_waveformWidget, &WaveformWidget::boundaryDragStarted, this, onDragStarted);
    connect(m_powerWidget, &PowerWidget::boundaryDragStarted, this, onDragStarted);
    connect(m_spectrogramWidget, &SpectrogramWidget::boundaryDragStarted, this, onDragStarted);

    // Drag finished → clear drag highlight
    auto onDragFinished = [this](int, int, TimePos) {
        m_boundaryOverlay->setDraggedBoundary(-1);
    };
    connect(m_waveformWidget, &WaveformWidget::boundaryDragFinished, this, onDragFinished);
    connect(m_powerWidget, &PowerWidget::boundaryDragFinished, this, onDragFinished);
    connect(m_spectrogramWidget, &SpectrogramWidget::boundaryDragFinished, this, onDragFinished);
}

} // namespace phonemelabeler
} // namespace dstools
