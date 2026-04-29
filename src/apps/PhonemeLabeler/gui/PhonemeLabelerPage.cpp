#include "PhonemeLabelerPage.h"
#include "ui/WaveformWidget.h"
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

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <dstools/AudioDecoder.h>
#include <dstools/WaveFormat.h>
#include <dstools/AudioFileResolver.h>
#include <dstools/ShortcutManager.h>

namespace dstools {
namespace phonemelabeler {

PhonemeLabelerPage::PhonemeLabelerPage(QWidget *parent)
    : QWidget(parent),
      m_playWidget(new dstools::widgets::PlayWidget()),
      m_document(new TextGridDocument(this)),
      m_undoStack(new QUndoStack(this)),
      m_viewport(new ViewportController(this)),
      m_bindingManager(new BoundaryBindingManager(m_document, this))
{
    buildActions();
    buildToolbar();
    buildLayout();
    connectSignals();

    m_shortcutManager = new dstools::widgets::ShortcutManager(&m_settings, this);
    m_shortcutManager->bind(m_actSave, PhonemeLabelerKeys::ShortcutSave, tr("Save"), tr("File"));
    m_shortcutManager->bind(m_actSaveAs, PhonemeLabelerKeys::ShortcutSaveAs, tr("Save As"), tr("File"));
    m_shortcutManager->bind(m_actUndo, PhonemeLabelerKeys::ShortcutUndo, tr("Undo"), tr("Edit"));
    m_shortcutManager->bind(m_actRedo, PhonemeLabelerKeys::ShortcutRedo, tr("Redo"), tr("Edit"));
    m_shortcutManager->bind(m_actZoomIn, PhonemeLabelerKeys::ShortcutZoomIn, tr("Zoom In"), tr("View"));
    m_shortcutManager->bind(m_actZoomOut, PhonemeLabelerKeys::ShortcutZoomOut, tr("Zoom Out"), tr("View"));
    m_shortcutManager->bind(m_actZoomReset, PhonemeLabelerKeys::ShortcutZoomReset, tr("Reset Zoom"), tr("View"));
    m_shortcutManager->bind(m_actToggleBinding, PhonemeLabelerKeys::ShortcutToggleBinding, tr("Toggle Binding"), tr("View"));
    m_shortcutManager->bind(m_actPlayPause, PhonemeLabelerKeys::PlaybackPlayPause, tr("Play/Pause"), tr("Playback"));
    m_shortcutManager->bind(m_actStop, PhonemeLabelerKeys::PlaybackStop, tr("Stop"), tr("Playback"));
    m_shortcutManager->applyAll();

    applyConfig();
}

PhonemeLabelerPage::~PhonemeLabelerPage() = default;

void PhonemeLabelerPage::setWorkingDirectory(const QString &dir) {
    if (dir.isEmpty()) return;
    m_settings.set(PhonemeLabelerKeys::LastDir, dir);
    m_currentDir = dir;
    m_fileListPanel->setDirectory(dir);
    emit workingDirectoryChanged(dir);

    // Load first TextGrid file if available
    auto files = m_fileListPanel->textGridFiles();
    if (!files.isEmpty()) {
        onFileSelected(files.first());
    }
}

void PhonemeLabelerPage::openFile(const QString &path) {
    onFileSelected(path);
}

QList<QAction *> PhonemeLabelerPage::editActions() const {
    return {m_actUndo, m_actRedo, nullptr, m_actSave, m_actSaveAs};
}

QList<QAction *> PhonemeLabelerPage::viewActions() const {
    return {m_actZoomIn, m_actZoomOut, m_actZoomReset, nullptr,
            m_actToggleBinding, nullptr,
            m_actTogglePower, m_actToggleSpectrogram};
}

bool PhonemeLabelerPage::hasUnsavedChanges() const {
    return m_document->isModified();
}

bool PhonemeLabelerPage::save() {
    return saveFile();
}

bool PhonemeLabelerPage::saveAs() {
    return saveFileAs();
}

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
        m_waveformWidget->updateBoundaryOverlay();
        m_powerWidget->updateBoundaryOverlay();
        m_spectrogramWidget->updateBoundaryOverlay();
        m_boundaryOverlay->update();
    });
    // Also repaint overlays when boundaries move (real-time during drag)
    connect(m_document, &TextGridDocument::boundaryMoved, this, [this](int, int, double) {
        m_waveformWidget->updateBoundaryOverlay();
        m_powerWidget->updateBoundaryOverlay();
        m_spectrogramWidget->updateBoundaryOverlay();
        m_boundaryOverlay->update();
        // Repaint all tier views so binding-state boundaries update in real-time
        m_tierEditWidget->update();
        for (auto *child : m_tierEditWidget->findChildren<QWidget *>()) {
            child->update();
        }
    });
    connect(m_document, &TextGridDocument::boundaryInserted, this, [this](int, int) {
        m_waveformWidget->updateBoundaryOverlay();
        m_powerWidget->updateBoundaryOverlay();
        m_spectrogramWidget->updateBoundaryOverlay();
        m_boundaryOverlay->update();
    });
    connect(m_document, &TextGridDocument::boundaryRemoved, this, [this](int, int) {
        m_waveformWidget->updateBoundaryOverlay();
        m_powerWidget->updateBoundaryOverlay();
        m_spectrogramWidget->updateBoundaryOverlay();
        m_boundaryOverlay->update();
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
    auto onDragFinished = [this](int, int, double) {
        m_boundaryOverlay->setDraggedBoundary(-1);
    };
    connect(m_waveformWidget, &WaveformWidget::boundaryDragFinished, this, onDragFinished);
    connect(m_powerWidget, &PowerWidget::boundaryDragFinished, this, onDragFinished);
    connect(m_spectrogramWidget, &SpectrogramWidget::boundaryDragFinished, this, onDragFinished);
}

void PhonemeLabelerPage::applyConfig() {
    m_bindingManager->setEnabled(m_settings.get(PhonemeLabelerKeys::BoundaryBindingEnabled));
    m_bindingManager->setTolerance(m_settings.get(PhonemeLabelerKeys::BoundaryBindingTolerance));
    m_viewport->setPixelsPerSecond(m_settings.get(PhonemeLabelerKeys::ZoomLevel));

    bool powerEnabled = m_settings.get(PhonemeLabelerKeys::PowerEnabled);
    m_powerWidget->setVisible(powerEnabled);
    m_actTogglePower->setChecked(powerEnabled);

    bool spectrogramEnabled = m_settings.get(PhonemeLabelerKeys::SpectrogramEnabled);
    m_spectrogramWidget->setVisible(spectrogramEnabled);
    m_actToggleSpectrogram->setChecked(spectrogramEnabled);

    // Apply saved spectrogram color style
    QString colorStyle = m_settings.get(PhonemeLabelerKeys::SpectrogramColorStyle);
    m_spectrogramWidget->setColorPalette(SpectrogramColorPalette::fromName(colorStyle));

    // Emit initial states
    emit zoomChanged(m_viewport->pixelsPerSecond());
    emit bindingChanged(m_bindingManager->isEnabled());
}

void PhonemeLabelerPage::onFileSelected(const QString &path) {
    if (!maybeSave()) return;

    if (!m_document->load(path)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to load file: %1").arg(path));
        return;
    }

    m_currentFilePath = path;
    m_settings.set(PhonemeLabelerKeys::LastDir, QFileInfo(path).absolutePath());

    // Load audio if available (same name, different extension)
    QString audioFilePath = dstools::AudioFileResolver::findAudioFile(path);
    if (!audioFilePath.isEmpty()) {
            m_waveformWidget->loadAudio(audioFilePath);
            m_playWidget->openFile(audioFilePath);

            // Load audio data for power and spectrogram widgets
            dstools::audio::AudioDecoder decoder;
            if (decoder.open(audioFilePath)) {
                auto fmt = decoder.format();
                int sampleRate = fmt.sampleRate();
                int channels = fmt.channels();

                std::vector<float> allSamples;
                const int bufferSize = 4096;
                std::vector<float> buffer(bufferSize);
                while (true) {
                    int read = decoder.read(buffer.data(), 0, bufferSize);
                    if (read <= 0) break;
                    allSamples.insert(allSamples.end(), buffer.begin(), buffer.begin() + read);
                }
                decoder.close();

                // Convert to mono
                std::vector<float> monoSamples;
                if (channels > 1) {
                    monoSamples.resize(allSamples.size() / channels);
                    for (size_t i = 0; i < monoSamples.size(); ++i) {
                        float sum = 0.0f;
                        for (int c = 0; c < channels; ++c) {
                            sum += allSamples[i * channels + c];
                        }
                        monoSamples[i] = sum / channels;
                    }
                } else {
                    monoSamples = std::move(allSamples);
                }

                m_powerWidget->setAudioData(monoSamples, sampleRate);
                m_spectrogramWidget->setAudioData(monoSamples, sampleRate);
            }
    }

    m_waveformWidget->setDocument(m_document);
    m_tierEditWidget->setDocument(m_document);
    m_powerWidget->setDocument(m_document);
    m_spectrogramWidget->setDocument(m_document);
    m_boundaryOverlay->setDocument(m_document);
    m_entryListPanel->setDocument(m_document);
    m_fileListPanel->setCurrentFile(path);

    // Set viewport to cover full duration
    double duration = m_document->totalDuration();
    m_viewport->setTotalDuration(duration);
    m_viewport->setViewRange(0.0, duration);
    updateScrollBar();

    emit fileSelected(path);
    emit fileStatusChanged(QFileInfo(path).fileName());
    emit documentLoaded(path);
}

bool PhonemeLabelerPage::saveFile() {
    if (m_currentFilePath.isEmpty()) {
        return saveFileAs();
    }
    if (!m_document->save(m_currentFilePath)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to save file: %1").arg(m_currentFilePath));
        return false;
    }
    emit modificationChanged(false);
    emit documentSaved(m_currentFilePath);
    return true;
}

bool PhonemeLabelerPage::saveFileAs() {
    QString path = QFileDialog::getSaveFileName(this, tr("Save TextGrid File"),
        m_currentFilePath.isEmpty() ? m_settings.get(PhonemeLabelerKeys::LastDir) : m_currentFilePath,
        tr("TextGrid Files (*.TextGrid);;All Files (*.*)"));
    if (path.isEmpty()) return false;

    if (!path.endsWith(".TextGrid", Qt::CaseInsensitive)) {
        path += ".TextGrid";
    }

    m_currentFilePath = path;
    return saveFile();
}

bool PhonemeLabelerPage::maybeSave() {
    if (!m_document->isModified()) return true;

    QMessageBox::StandardButton ret = QMessageBox::warning(this, tr("Save Changes"),
        tr("The document has been modified.\nDo you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    if (ret == QMessageBox::Save) {
        return saveFile();
    } else if (ret == QMessageBox::Cancel) {
        return false;
    }
    return true;
}

void PhonemeLabelerPage::onZoomIn() {
    m_viewport->zoomAt(m_viewport->viewCenter(), 1.25);
    m_settings.set(PhonemeLabelerKeys::ZoomLevel, m_viewport->pixelsPerSecond());
    emit zoomChanged(m_viewport->pixelsPerSecond());
}

void PhonemeLabelerPage::onZoomOut() {
    m_viewport->zoomAt(m_viewport->viewCenter(), 0.8);
    m_settings.set(PhonemeLabelerKeys::ZoomLevel, m_viewport->pixelsPerSecond());
    emit zoomChanged(m_viewport->pixelsPerSecond());
}

void PhonemeLabelerPage::onZoomReset() {
    m_viewport->setPixelsPerSecond(200.0);
    m_settings.set(PhonemeLabelerKeys::ZoomLevel, 200.0);
    emit zoomChanged(200.0);
}

void PhonemeLabelerPage::onPlayPause() {
    if (m_playWidget->isPlaying()) {
        m_playWidget->setPlaying(false);
    } else {
        m_playWidget->setPlaying(true);
    }
}

void PhonemeLabelerPage::onStop() {
    m_playWidget->setPlaying(false);
}

void PhonemeLabelerPage::updatePlaybackState() {
    const bool playing = m_playWidget->isPlaying();
    if (playing) {
        m_actPlayPause->setIcon(QIcon(":/icons/pause.svg"));
        m_actPlayPause->setText(tr("Pause"));
    } else {
        m_actPlayPause->setIcon(QIcon(":/icons/play.svg"));
        m_actPlayPause->setText(tr("Play"));
    }
}

void PhonemeLabelerPage::updateScrollBar() {
    double totalDuration = m_viewport->totalDuration();
    if (totalDuration <= 0.0) {
        m_hScrollBar->setRange(0, 0);
        return;
    }

    // Use milliseconds as scroll units for precision
    double viewDuration = m_viewport->duration();
    int totalMs = static_cast<int>(totalDuration * 1000.0);
    int viewMs = static_cast<int>(viewDuration * 1000.0);
    int maxVal = qMax(0, totalMs - viewMs);

    // Block signals to avoid feedback loop
    QSignalBlocker blocker(m_hScrollBar);
    m_hScrollBar->setRange(0, maxVal);
    m_hScrollBar->setPageStep(viewMs);
    m_hScrollBar->setSingleStep(qMax(1, viewMs / 10));
    m_hScrollBar->setValue(static_cast<int>(m_viewport->startSec() * 1000.0));
}

void PhonemeLabelerPage::onScrollBarValueChanged(int value) {
    double startSec = value / 1000.0;
    double viewDuration = m_viewport->duration();
    m_viewport->setViewRange(startSec, startSec + viewDuration);
}

void PhonemeLabelerPage::playBoundarySegment(double timeSec) {
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
