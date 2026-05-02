#include "PhonemeLabelerPage.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QVBoxLayout>

#include <dstools/AudioFileResolver.h>
#include <dstools/ShortcutEditorWidget.h>
#include <dstools/ShortcutManager.h>
#include <dsfw/Theme.h>

namespace dstools {
namespace phonemelabeler {

PhonemeLabelerPage::PhonemeLabelerPage(QWidget *parent)
    : QWidget(parent),
      m_editor(new PhonemeEditor(this)),
      m_fileListPanel(new FileListPanel(this))
{
    // Layout: file list | editor
    m_outerSplitter = new QSplitter(Qt::Horizontal, this);
    m_outerSplitter->addWidget(m_fileListPanel);
    m_outerSplitter->addWidget(m_editor);
    m_outerSplitter->setStretchFactor(0, 0); // file list: fixed
    m_outerSplitter->setStretchFactor(1, 1); // editor: stretch

    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(m_outerSplitter);

    // Shortcuts
    m_shortcutManager = new dstools::widgets::ShortcutManager(&m_settings, this);
    m_shortcutManager->bind(m_editor->saveAction(), PhonemeLabelerKeys::ShortcutSave, tr("Save"), tr("File"));
    m_shortcutManager->bind(m_editor->saveAsAction(), PhonemeLabelerKeys::ShortcutSaveAs, tr("Save As"), tr("File"));
    m_shortcutManager->bind(m_editor->undoAction(), PhonemeLabelerKeys::ShortcutUndo, tr("Undo"), tr("Edit"));
    m_shortcutManager->bind(m_editor->redoAction(), PhonemeLabelerKeys::ShortcutRedo, tr("Redo"), tr("Edit"));
    m_shortcutManager->bind(m_editor->zoomInAction(), PhonemeLabelerKeys::ShortcutZoomIn, tr("Zoom In"), tr("View"));
    m_shortcutManager->bind(m_editor->zoomOutAction(), PhonemeLabelerKeys::ShortcutZoomOut, tr("Zoom Out"), tr("View"));
    m_shortcutManager->bind(m_editor->zoomResetAction(), PhonemeLabelerKeys::ShortcutZoomReset, tr("Reset Zoom"), tr("View"));
    m_shortcutManager->bind(m_editor->toggleBindingAction(), PhonemeLabelerKeys::ShortcutToggleBinding, tr("Toggle Binding"), tr("View"));
    m_shortcutManager->bind(m_editor->playPauseAction(), PhonemeLabelerKeys::PlaybackPlayPause, tr("Play/Pause"), tr("Playback"));
    m_shortcutManager->bind(m_editor->stopAction(), PhonemeLabelerKeys::PlaybackStop, tr("Stop"), tr("Playback"));
    m_shortcutManager->applyAll();

    // Connect save actions to page-level file operations
    connect(m_editor->saveAction(), &QAction::triggered, this, &PhonemeLabelerPage::saveFile);
    connect(m_editor->saveAsAction(), &QAction::triggered, this, &PhonemeLabelerPage::saveFileAs);

    // File list selection
    connect(m_fileListPanel, &FileListPanel::fileSelected, this, &PhonemeLabelerPage::onFileSelected);

    // Forward editor signals
    connect(m_editor, &PhonemeEditor::zoomChanged, this, [this](double pps) {
        m_settings.set(PhonemeLabelerKeys::ZoomLevel, pps);
    });
    connect(m_editor, &PhonemeEditor::bindingChanged, this, [this](bool enabled) {
        m_settings.set(PhonemeLabelerKeys::BoundaryBindingEnabled, enabled);
    });

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

bool PhonemeLabelerPage::hasUnsavedChanges() const {
    return m_editor->document()->isModified();
}

bool PhonemeLabelerPage::save() {
    return saveFile();
}

bool PhonemeLabelerPage::saveAs() {
    return saveFileAs();
}

void PhonemeLabelerPage::applyConfig() {
    m_editor->setBindingEnabled(m_settings.get(PhonemeLabelerKeys::BoundaryBindingEnabled));
    m_editor->setBindingToleranceMs(m_settings.get(PhonemeLabelerKeys::BoundaryBindingTolerance));
    m_editor->setPixelsPerSecond(m_settings.get(PhonemeLabelerKeys::ZoomLevel));

    m_editor->setPowerVisible(m_settings.get(PhonemeLabelerKeys::PowerEnabled));
    m_editor->setSpectrogramVisible(m_settings.get(PhonemeLabelerKeys::SpectrogramEnabled));

    QString colorStyle = m_settings.get(PhonemeLabelerKeys::SpectrogramColorStyle);
    m_editor->setSpectrogramColorStyle(colorStyle);
}

void PhonemeLabelerPage::onFileSelected(const QString &path) {
    if (!maybeSave()) return;

    if (!m_editor->document()->load(path)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to load file: %1").arg(path));
        return;
    }

    m_currentFilePath = path;
    m_settings.set(PhonemeLabelerKeys::LastDir, QFileInfo(path).absolutePath());

    // Load audio if available
    QString audioFilePath = dstools::AudioFileResolver::findAudioFile(path);
    if (!audioFilePath.isEmpty()) {
        m_editor->loadAudio(audioFilePath);
    }

    m_fileListPanel->setCurrentFile(path);

    emit fileSelected(path);
    emit documentLoaded(path);
}

bool PhonemeLabelerPage::saveFile() {
    if (m_currentFilePath.isEmpty()) {
        return saveFileAs();
    }
    if (!m_editor->document()->save(m_currentFilePath)) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to save file: %1").arg(m_currentFilePath));
        return false;
    }
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
    if (!m_editor->document()->isModified()) return true;

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
    fileMenu->addAction(m_editor->saveAction());
    fileMenu->addAction(m_editor->saveAsAction());
    fileMenu->addSeparator();

    auto *actExit = fileMenu->addAction(tr("E&xit"), [parent]() {
        if (auto *w = parent->window())
            w->close();
    });

    m_shortcutManager->bind(actOpenDir, PhonemeLabelerKeys::ShortcutOpen, tr("Open Directory"), tr("File"));
    m_shortcutManager->bind(actExit, PhonemeLabelerKeys::ShortcutExit, tr("Exit"), tr("File"));
    m_shortcutManager->applyAll();

    // Edit menu
    auto *editMenu = menuBar->addMenu(tr("&Edit"));
    editMenu->addAction(m_editor->undoAction());
    editMenu->addAction(m_editor->redoAction());

    // View menu
    auto *viewMenu = menuBar->addMenu(tr("&View"));
    for (auto *act : m_editor->viewActions()) {
        if (act)
            viewMenu->addAction(act);
        else
            viewMenu->addSeparator();
    }
    viewMenu->addMenu(m_editor->spectrogramColorMenu());

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

    connect(m_editor, &PhonemeEditor::fileStatusChanged, fileLabel, &QLabel::setText);
    connect(m_editor, &PhonemeEditor::positionChanged, this, [posLabel](double sec) {
        posLabel->setText(QString::number(sec, 'f', 3) + "s");
    });
    connect(m_editor, &PhonemeEditor::zoomChanged, this, [zoomLabel](double pps) {
        zoomLabel->setText(QString::number(pps, 'f', 1) + " px/s");
    });
    connect(m_editor, &PhonemeEditor::bindingChanged, this, [bindLabel](bool enabled) {
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
