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

#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <dstools/AudioDecoder.h>
#include <dstools/WaveFormat.h>
#include <dstools/AudioFileResolver.h>
#include <dstools/ShortcutManager.h>
#include <dstools/ShortcutEditorWidget.h>
#include <dsfw/Theme.h>

namespace dstools {
namespace phonemelabeler {

PhonemeLabelerPage::PhonemeLabelerPage(QWidget *parent)
    : QWidget(parent),
      m_playWidget(new dstools::widgets::PlayWidget()),
      m_document(new TextGridDocument(this)),
      m_undoStack(new QUndoStack(this)),
      m_viewport(new ViewportController(this)),
      m_bindingManager(new BoundaryBindingManager(m_document, this)),
      m_renderer(new WaveformRenderer(this))
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

void PhonemeLabelerPage::updateAllBoundaryOverlays() {
    m_waveformWidget->updateBoundaryOverlay();
    m_powerWidget->updateBoundaryOverlay();
    m_spectrogramWidget->updateBoundaryOverlay();
    m_boundaryOverlay->update();
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

            if (m_renderer->loadAudio(audioFilePath)) {
                m_powerWidget->setAudioData(m_renderer->samples(), m_renderer->sampleRate());
                m_spectrogramWidget->setAudioData(m_renderer->samples(), m_renderer->sampleRate());
            } else {
                qWarning() << "PhonemeLabeler: Failed to decode audio for power/spectrogram:" << audioFilePath;
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

QMenuBar *PhonemeLabelerPage::createMenuBar(QWidget *parent) {
    auto *menuBar = new QMenuBar(parent);

    // File menu
    auto *fileMenu = menuBar->addMenu(tr("&File"));

    auto *actOpenDir = fileMenu->addAction(tr("Open &Directory..."), [this]() {
        QString dir = QFileDialog::getExistingDirectory(
            this, tr("Open Directory"), m_settings.get(PhonemeLabelerKeys::LastDir));
        if (!dir.isEmpty()) {
            setWorkingDirectory(dir);
        }
    });

    fileMenu->addAction(tr("Open &File..."), [this]() {
        QString path = QFileDialog::getOpenFileName(
            this, tr("Open TextGrid File"),
            m_settings.get(PhonemeLabelerKeys::LastDir),
            tr("TextGrid Files (*.TextGrid *.textgrid);;All Files (*.*)"));
        if (!path.isEmpty()) {
            openFile(path);
        }
    });

    fileMenu->addSeparator();
    fileMenu->addAction(m_actSave);
    fileMenu->addAction(m_actSaveAs);
    fileMenu->addSeparator();

    auto *actExit = fileMenu->addAction(tr("E&xit"), [parent]() {
        if (auto *w = parent->window())
            w->close();
    });

    // Bind menu-owned shortcuts
    m_shortcutManager->bind(actOpenDir, PhonemeLabelerKeys::ShortcutOpen, tr("Open Directory"), tr("File"));
    m_shortcutManager->bind(actExit, PhonemeLabelerKeys::ShortcutExit, tr("Exit"), tr("File"));
    m_shortcutManager->applyAll();

    // Edit menu
    auto *editMenu = menuBar->addMenu(tr("&Edit"));
    editMenu->addAction(m_actUndo);
    editMenu->addAction(m_actRedo);

    // View menu
    auto *viewMenu = menuBar->addMenu(tr("&View"));
    for (auto *act : viewActions()) {
        if (act)
            viewMenu->addAction(act);
        else
            viewMenu->addSeparator();
    }
    viewMenu->addMenu(m_spectrogramColorMenu);

    viewMenu->addSeparator();
    dsfw::Theme::instance().populateThemeMenu(viewMenu);

    viewMenu->addSeparator();
    viewMenu->addAction(tr("&Keyboard Shortcuts..."), [this]() {
        m_shortcutManager->showEditor(this);
    });

    // Help menu
    auto *helpMenu = menuBar->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), [this]() {
        QMessageBox::about(this, tr("About PhonemeLabeler"),
            tr("<h3>PhonemeLabeler 0.1.0</h3>"
               "<p>A TextGrid editor for DiffSinger dataset preparation.</p>"
               "<p>Built with Qt %1 and C++17.</p>")
               .arg(QT_VERSION_STR));
    });

    return menuBar;
}

QWidget *PhonemeLabelerPage::createStatusBarContent(QWidget *parent) {
    auto *container = new QWidget(parent);
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *fileLabel = new QLabel(tr("No file"), container);
    auto *posLabel = new QLabel(QStringLiteral("0.000s"), container);
    auto *zoomLabel = new QLabel(QStringLiteral("200.0 px/s"), container);
    auto *bindLabel = new QLabel(tr("Binding: ON"), container);

    layout->addWidget(fileLabel, 1);
    layout->addWidget(posLabel);
    layout->addWidget(zoomLabel);
    layout->addWidget(bindLabel);

    connect(this, &PhonemeLabelerPage::fileStatusChanged, fileLabel, &QLabel::setText);
    connect(this, &PhonemeLabelerPage::positionChanged, this, [posLabel](double sec) {
        posLabel->setText(QString::number(sec, 'f', 3) + "s");
    });
    connect(this, &PhonemeLabelerPage::zoomChanged, this, [zoomLabel](double pps) {
        zoomLabel->setText(QString::number(pps, 'f', 1) + " px/s");
    });
    connect(this, &PhonemeLabelerPage::bindingChanged, this, [bindLabel](bool enabled) {
        bindLabel->setText(QStringLiteral("Binding: %1").arg(enabled ? "ON" : "OFF"));
    });

    return container;
}

QString PhonemeLabelerPage::windowTitle() const {
    QString title = tr("PhonemeLabeler");
    if (!m_currentFilePath.isEmpty()) {
        title += " - " + QFileInfo(m_currentFilePath).fileName();
        if (hasUnsavedChanges()) {
            title += " *";
        }
    }
    return title;
}

} // namespace phonemelabeler
} // namespace dstools
