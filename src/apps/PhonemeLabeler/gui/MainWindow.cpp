#include "MainWindow.h"
#include "ui/WaveformWidget.h"
#include "ui/TierEditWidget.h"
#include "ui/PowerWidget.h"
#include "ui/SpectrogramWidget.h"
#include "ui/SpectrogramColorPalette.h"
#include "ui/FileListPanel.h"
#include "ui/EntryListPanel.h"
#include "ui/TimeRulerWidget.h"
#include "ui/TextGridDocument.h"
#include "ui/ViewportController.h"
#include "ui/BoundaryBindingManager.h"
#include "ui/BoundaryOverlayWidget.h"

#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QScrollBar>
#include <QSettings>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QKeySequence>

#include <dstools/AudioDecoder.h>
#include <dstools/WaveFormat.h>

namespace dstools {
namespace phonemelabeler {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    m_playWidget(new dstools::widgets::PlayWidget()),
    m_document(new TextGridDocument(this)),
    m_undoStack(new QUndoStack(this)),
    m_viewport(new ViewportController(this)),
    m_bindingManager(new BoundaryBindingManager(m_document, this))
{
    buildMenuBar();
    buildToolbar();
    buildCentralLayout();
    buildStatusBar();
    connectSignals();
    applyConfig();

    // Restore window state via native QSettings
    QSettings settings("PhonemeLabeler", "PhonemeLabeler");
    auto geom = settings.value("geometry").toByteArray();
    if (!geom.isEmpty()) {
        restoreGeometry(geom);
    }
    auto state = settings.value("windowState").toByteArray();
    if (!state.isEmpty()) {
        restoreState(state);
    }

    updateWindowTitle();
    updateUndoRedoState();
    updateZoomStatus();
    updateBindingStatus();
}

MainWindow::~MainWindow() {
    QSettings settings("PhonemeLabeler", "PhonemeLabeler");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
}

void MainWindow::openFile(const QString &path) {
    onFileSelected(path);
}

void MainWindow::buildMenuBar() {
    // File menu
    m_fileMenu = menuBar()->addMenu(tr("&File"));

    m_actOpenDir = m_fileMenu->addAction(tr("Open &Directory..."), this, &MainWindow::openDirectory);
    m_actOpenDir->setShortcut(m_settings.shortcut(PhonemeLabelerKeys::ShortcutOpen));

    m_actOpenFile = m_fileMenu->addAction(tr("Open &File..."), [this]() {
        QString path = QFileDialog::getOpenFileName(this, tr("Open TextGrid File"),
            m_settings.get(PhonemeLabelerKeys::LastDir),
            tr("TextGrid Files (*.TextGrid *.textgrid);;All Files (*.*)"));
        if (!path.isEmpty()) {
            onFileSelected(path);
        }
    });
    m_fileMenu->addSeparator();

    m_actSave = m_fileMenu->addAction(tr("&Save"), this, &MainWindow::saveFile);
    m_actSave->setShortcut(m_settings.shortcut(PhonemeLabelerKeys::ShortcutSave));

    m_actSaveAs = m_fileMenu->addAction(tr("Save &As..."), this, &MainWindow::saveFileAs);
    m_actSaveAs->setShortcut(m_settings.shortcut(PhonemeLabelerKeys::ShortcutSaveAs));

    m_fileMenu->addSeparator();
    m_actExit = m_fileMenu->addAction(tr("E&xit"), this, &QMainWindow::close);
    m_actExit->setShortcut(m_settings.shortcut(PhonemeLabelerKeys::ShortcutExit));

    // Edit menu
    m_editMenu = menuBar()->addMenu(tr("&Edit"));

    m_actUndo = m_editMenu->addAction(tr("&Undo"), [this]() {
        if (m_undoStack->canUndo()) m_undoStack->undo();
    });
    m_actUndo->setShortcut(m_settings.shortcut(PhonemeLabelerKeys::ShortcutUndo));

    m_actRedo = m_editMenu->addAction(tr("&Redo"), [this]() {
        if (m_undoStack->canRedo()) m_undoStack->redo();
    });
    m_actRedo->setShortcut(m_settings.shortcut(PhonemeLabelerKeys::ShortcutRedo));

    // View menu
    m_viewMenu = menuBar()->addMenu(tr("&View"));

    m_actZoomIn = m_viewMenu->addAction(tr("Zoom &In"), this, &MainWindow::onZoomIn);
    m_actZoomIn->setShortcut(m_settings.shortcut(PhonemeLabelerKeys::ShortcutZoomIn));

    m_actZoomOut = m_viewMenu->addAction(tr("Zoom &Out"), this, &MainWindow::onZoomOut);
    m_actZoomOut->setShortcut(m_settings.shortcut(PhonemeLabelerKeys::ShortcutZoomOut));

    m_actZoomReset = m_viewMenu->addAction(tr("&Reset Zoom"), this, &MainWindow::onZoomReset);
    m_actZoomReset->setShortcut(m_settings.shortcut(PhonemeLabelerKeys::ShortcutZoomReset));

    m_viewMenu->addSeparator();

    m_actToggleBinding = m_viewMenu->addAction(tr("&Boundary Binding"), this, [this]() {
        bool enabled = !m_bindingManager->isEnabled();
        m_bindingManager->setEnabled(enabled);
        m_settings.set(PhonemeLabelerKeys::BoundaryBindingEnabled, enabled);
        updateBindingStatus();
    });
    m_actToggleBinding->setCheckable(true);
    m_actToggleBinding->setShortcut(m_settings.shortcut(PhonemeLabelerKeys::ShortcutToggleBinding));

    m_viewMenu->addSeparator();

    m_actTogglePower = m_viewMenu->addAction(tr("Show &Power"), this, [this]() {
        bool enabled = m_actTogglePower->isChecked();
        m_powerWidget->setVisible(enabled);
        m_settings.set(PhonemeLabelerKeys::PowerEnabled, enabled);
    });
    m_actTogglePower->setCheckable(true);

    m_actToggleSpectrogram = m_viewMenu->addAction(tr("Show &Spectrogram"), this, [this]() {
        bool enabled = m_actToggleSpectrogram->isChecked();
        m_spectrogramWidget->setVisible(enabled);
        m_settings.set(PhonemeLabelerKeys::SpectrogramEnabled, enabled);
    });
    m_actToggleSpectrogram->setCheckable(true);

    // Spectrogram color style submenu
    m_spectrogramColorMenu = m_viewMenu->addMenu(tr("Spectrogram &Color"));
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

    // Help menu
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_actAbout = m_helpMenu->addAction(tr("&About"), [this]() {
        QMessageBox::about(this, tr("About PhonemeLabeler"),
            tr("<h3>PhonemeLabeler 0.1.0</h3>"
               "<p>A TextGrid editor for DiffSinger dataset preparation.</p>"
               "<p>Built with Qt %1 and C++17.</p>")
               .arg(QT_VERSION_STR));
    });
}

void MainWindow::buildToolbar() {
    QToolBar *toolbar = addToolBar(tr("Main Toolbar"));
    toolbar->setMovable(false);

    m_actPlayPause = toolbar->addAction(tr("Play"), this, &MainWindow::onPlayPause);
    m_actPlayPause->setShortcut(m_settings.shortcut(PhonemeLabelerKeys::PlaybackPlayPause));

    m_actStop = toolbar->addAction(tr("Stop"), this, &MainWindow::onStop);
    m_actStop->setShortcut(m_settings.shortcut(PhonemeLabelerKeys::PlaybackStop));

    toolbar->addSeparator();

    m_actBindingToggle = toolbar->addAction(tr("Binding"), this, [this]() {
        bool enabled = !m_bindingManager->isEnabled();
        m_bindingManager->setEnabled(enabled);
        m_settings.set(PhonemeLabelerKeys::BoundaryBindingEnabled, enabled);
        updateBindingStatus();
        m_actToggleBinding->setChecked(enabled);
    });
    m_actBindingToggle->setCheckable(true);

    toolbar->addSeparator();
    toolbar->addWidget(m_playWidget);
}

void MainWindow::buildCentralLayout() {
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(m_mainSplitter);

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

    // Tier editor (标签区域)
    m_tierEditWidget = new TierEditWidget(m_document, m_undoStack, m_viewport, m_bindingManager, this);
    m_rightSplitter->addWidget(m_tierEditWidget);

    // Waveform (波形图)
    m_waveformWidget = new WaveformWidget(m_viewport, this);
    m_waveformWidget->setBindingManager(m_bindingManager);
    m_waveformWidget->setUndoStack(m_undoStack);
    m_waveformWidget->setPlayWidget(m_playWidget);
    m_rightSplitter->addWidget(m_waveformWidget);

    // Power (power曲线图)
    m_powerWidget = new PowerWidget(m_viewport, this);
    m_powerWidget->setBindingManager(m_bindingManager);
    m_powerWidget->setUndoStack(m_undoStack);
    m_rightSplitter->addWidget(m_powerWidget);

    // Spectrogram (频谱)
    m_spectrogramWidget = new SpectrogramWidget(m_viewport, this);
    m_spectrogramWidget->setBindingManager(m_bindingManager);
    m_spectrogramWidget->setUndoStack(m_undoStack);
    m_rightSplitter->addWidget(m_spectrogramWidget);

    m_rightSplitter->setStretchFactor(0, 0); // tier: fixed
    m_rightSplitter->setStretchFactor(1, 1); // waveform: 1/4
    m_rightSplitter->setStretchFactor(2, 1); // power: 1/4
    m_rightSplitter->setStretchFactor(3, 2); // spectrogram: 1/2
    m_rightSplitter->setHandleWidth(1);

    // Boundary overlay: spans the entire splitter area for continuous lines
    m_boundaryOverlay = new BoundaryOverlayWidget(m_viewport, centerContainer);
    m_boundaryOverlay->trackWidget(m_rightSplitter);

    // Horizontal scrollbar at bottom
    m_hScrollBar = new QScrollBar(Qt::Horizontal, this);
    m_hScrollBar->setMinimum(0);
    m_hScrollBar->setMaximum(0);
    centerLayout->addWidget(m_hScrollBar);

    m_mainSplitter->addWidget(centerContainer);

    // Right: entry list panel (音素条目)
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

void MainWindow::buildStatusBar() {
    m_statusFile = new QLabel(tr("No file"), this);
    statusBar()->addWidget(m_statusFile);

    m_statusPosition = new QLabel("0.000s", this);
    statusBar()->addPermanentWidget(m_statusPosition);

    m_statusZoom = new QLabel("200.0 px/s", this);
    statusBar()->addPermanentWidget(m_statusZoom);

    m_statusBinding = new QLabel(tr("Binding: ON"), this);
    statusBar()->addPermanentWidget(m_statusBinding);
}

void MainWindow::connectSignals() {
    // Document signals
    connect(m_document, &TextGridDocument::documentChanged, this, &MainWindow::updateWindowTitle);
    connect(m_document, &TextGridDocument::documentChanged, this, &MainWindow::updateFileStatus);

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
    connect(m_hScrollBar, &QScrollBar::valueChanged, this, &MainWindow::onScrollBarValueChanged);

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
    connect(m_fileListPanel, &FileListPanel::fileSelected, this, &MainWindow::onFileSelected);

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
                m_statusPosition->setText(QString::number(sec, 'f', 3) + "s");
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

void MainWindow::applyConfig() {
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
}

void MainWindow::updateWindowTitle() {
    QString title = tr("PhonemeLabeler");
    if (!m_currentFilePath.isEmpty()) {
        title += " - " + QFileInfo(m_currentFilePath).fileName();
        if (m_document->isModified()) {
            title += " *";
        }
    }
    setWindowTitle(title);
}

void MainWindow::updateUndoRedoState() {
    m_actUndo->setEnabled(m_undoStack->canUndo());
    m_actRedo->setEnabled(m_undoStack->canRedo());
}

void MainWindow::updateZoomStatus() {
    m_statusZoom->setText(QString::number(m_viewport->pixelsPerSecond(), 'f', 1) + " px/s");
}

void MainWindow::updateBindingStatus() {
    bool enabled = m_bindingManager->isEnabled();
    m_statusBinding->setText(tr("Binding: %1").arg(enabled ? "ON" : "OFF"));
    if (m_actToggleBinding) m_actToggleBinding->setChecked(enabled);
    if (m_actBindingToggle) m_actBindingToggle->setChecked(enabled);
}

void MainWindow::updateFileStatus() {
    if (m_currentFilePath.isEmpty()) {
        m_statusFile->setText(tr("No file"));
    } else {
        m_statusFile->setText(QFileInfo(m_currentFilePath).fileName());
    }
}

void MainWindow::openDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
        m_settings.get(PhonemeLabelerKeys::LastDir));
    if (dir.isEmpty()) return;

    m_settings.set(PhonemeLabelerKeys::LastDir, dir);
    m_currentDir = dir;
    m_fileListPanel->setDirectory(dir);

    // Load first TextGrid file if available
    auto files = m_fileListPanel->textGridFiles();
    if (!files.isEmpty()) {
        onFileSelected(files.first());
    }
}

void MainWindow::onFileSelected(const QString &path) {
    if (!maybeSave()) return;

    if (!m_document->load(path)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to load file: %1").arg(path));
        return;
    }

    m_currentFilePath = path;
    m_settings.set(PhonemeLabelerKeys::LastDir, QFileInfo(path).absolutePath());

    // Load audio if available (same name, different extension)
    QStringList audioExts = {"wav", "mp3", "m4a", "flac", "ogg"};
    for (const QString &ext : audioExts) {
        QString tryPath = QFileInfo(path).absolutePath() + "/" + QFileInfo(path).completeBaseName() + "." + ext;
        if (QFile::exists(tryPath)) {
            m_waveformWidget->loadAudio(tryPath);
            m_playWidget->openFile(tryPath);

            // Load audio data for power and spectrogram widgets
            dstools::audio::AudioDecoder decoder;
            if (decoder.open(tryPath)) {
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
            break;
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

    updateWindowTitle();
    updateFileStatus();
    emit documentLoaded(path);
}

bool MainWindow::saveFile() {
    if (m_currentFilePath.isEmpty()) {
        return saveFileAs();
    }
    if (!m_document->save(m_currentFilePath)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to save file: %1").arg(m_currentFilePath));
        return false;
    }
    updateWindowTitle();
    emit documentSaved(m_currentFilePath);
    return true;
}

bool MainWindow::saveFileAs() {
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

bool MainWindow::maybeSave() {
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

void MainWindow::onZoomIn() {
    m_viewport->zoomAt(m_viewport->viewCenter(), 1.25);
    m_settings.set(PhonemeLabelerKeys::ZoomLevel, m_viewport->pixelsPerSecond());
    updateZoomStatus();
}

void MainWindow::onZoomOut() {
    m_viewport->zoomAt(m_viewport->viewCenter(), 0.8);
    m_settings.set(PhonemeLabelerKeys::ZoomLevel, m_viewport->pixelsPerSecond());
    updateZoomStatus();
}

void MainWindow::onZoomReset() {
    m_viewport->setPixelsPerSecond(200.0);
    m_settings.set(PhonemeLabelerKeys::ZoomLevel, 200.0);
    updateZoomStatus();
}

void MainWindow::onPlayPause() {
    if (m_playWidget->isPlaying()) {
        m_playWidget->setPlaying(false);
    } else {
        m_playWidget->setPlaying(true);
    }
}

void MainWindow::onStop() {
    m_playWidget->setPlaying(false);
}

void MainWindow::updatePlaybackState() {
    // Update play/pause icon based on state
}

void MainWindow::updateScrollBar() {
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

void MainWindow::onScrollBarValueChanged(int value) {
    double startSec = value / 1000.0;
    double viewDuration = m_viewport->duration();
    m_viewport->setViewRange(startSec, startSec + viewDuration);
}

void MainWindow::playBoundarySegment(double timeSec) {
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

void MainWindow::closeEvent(QCloseEvent *event) {
    if (maybeSave()) {
        QSettings settings("PhonemeLabeler", "PhonemeLabeler");
        settings.setValue("geometry", saveGeometry());
        settings.setValue("windowState", saveState());
        event->accept();
    } else {
        event->ignore();
    }
}

} // namespace phonemelabeler
} // namespace dstools
