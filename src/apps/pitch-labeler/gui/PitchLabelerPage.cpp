#include "PitchLabelerPage.h"

#include "DSFile.h"
#include "PitchFileService.h"
#include "../PitchLabelerKeys.h"
#include "ui/FileListPanel.h"
#include "ui/PianoRollView.h"
#include "ui/PropertyPanel.h"

#include <dstools/TimePos.h>

#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QMessageBox>
#include <QShortcut>

#include <dstools/ShortcutManager.h>
#include <dstools/AudioFileResolver.h>

#include <functional>

namespace dstools {
    namespace pitchlabeler {

        PitchLabelerPage::PitchLabelerPage(QWidget *parent)
            : QWidget(parent), m_fileService(new PitchFileService(this)),
              m_playWidget(new dstools::widgets::PlayWidget()),
              m_undoStack(new QUndoStack(this)) {

            m_viewport = new dstools::widgets::ViewportController(this);
            m_viewport->setPixelsPerSecond(100.0);

            buildActions();
            buildLayout();
            connectSignals();

            m_shortcutManager = new dstools::widgets::ShortcutManager(&m_settings, this);

            applyConfig();
        }

        PitchLabelerPage::~PitchLabelerPage() = default;

        void PitchLabelerPage::setWorkingDirectory(const QString &dir) {
            m_workingDirectory = dir;
            m_fileListPanel->setDirectory(dir);
            m_settings.set(PitchLabelerKeys::LastDir, dir);
            emit workingDirectoryChanged(dir);
        }

        void PitchLabelerPage::closeDirectory() {
            if (m_fileService->hasUnsavedChanges()) {
                auto ret = QMessageBox::question(this, QStringLiteral("未保存的更改"),
                                                 QStringLiteral("关闭前是否保存？"),
                                                 QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
                if (ret == QMessageBox::Save) {
                    m_fileService->saveFile();
                } else if (ret == QMessageBox::Cancel) {
                    return;
                }
            }

            m_fileListPanel->saveState();

            m_workingDirectory.clear();
            m_currentFilePath.clear();
            m_currentFile.reset();

            // Switch to empty state
            m_mainStack->setCurrentIndex(0);
            m_fileListPanel->clear();
            m_pianoRoll->clear();
            m_propertyPanel->clear();

            updateStatusInfo();
            emit workingDirectoryChanged(QString());
        }

        bool PitchLabelerPage::hasUnsavedChanges() const {
            return m_fileService->hasUnsavedChanges();
        }

        bool PitchLabelerPage::save() {
            return m_fileService->saveFile();
        }

        bool PitchLabelerPage::saveAll() {
            return m_fileService->saveAllFiles();
        }

        QString PitchLabelerPage::windowTitle() const {
            QString title = QStringLiteral("DiffSinger 音高标注器");
            if (!m_workingDirectory.isEmpty() && !m_currentFilePath.isEmpty()) {
                QString fileName = QFileInfo(m_currentFilePath).fileName();
                title = fileName + (hasUnsavedChanges() ? " *" : "") + " - " + title;
            }
            return title;
        }

        void PitchLabelerPage::openDirectory() {
            QString dir =
                QFileDialog::getExistingDirectory(this, QStringLiteral("打开工作目录"), QString(), QFileDialog::ShowDirsOnly);
            if (dir.isEmpty())
                return;
            setWorkingDirectory(dir);
        }

        void PitchLabelerPage::saveConfig() {
            if (!m_workingDirectory.isEmpty()) {
                m_settings.set(PitchLabelerKeys::LastDir, m_workingDirectory);
            }
            m_pianoRoll->pullConfig(m_settings);
            m_fileListPanel->saveState();
        }

        void PitchLabelerPage::applyConfig() {
            // Apply saved shortcuts
            m_shortcutManager->applyAll();

            // Window-level shortcuts for tool modes, playback, navigation
            rebuildWindowShortcuts();

            // Load piano roll display options
            m_pianoRoll->loadConfig(m_settings);

            // Restore last directory
            if (const QString savedDir = m_settings.get(PitchLabelerKeys::LastDir);
                !savedDir.isEmpty() && QDir(savedDir).exists()) {
                m_workingDirectory = savedDir;
                m_fileListPanel->setDirectory(savedDir);
            }
        }

        void PitchLabelerPage::loadFile(const QString &path) {
            if (!m_fileService->loadFile(path))
                return;

            m_currentFile = m_fileService->currentFile();
            m_currentFilePath = m_fileService->currentFilePath();

            m_undoStack->clear();
            m_mainStack->setCurrentIndex(1);
            m_pianoRoll->setDSFile(m_currentFile);
            m_propertyPanel->setDSFile(m_currentFile);
            m_actSave->setEnabled(true);

            QString audioPath = dstools::AudioFileResolver::findAudioFile(path);
            if (!audioPath.isEmpty()) {
                m_playWidget->openFile(audioPath);
            }

            if (m_pianoRoll) {
                double audioDur = m_playWidget->duration();
                if (audioDur > 0)
                    m_pianoRoll->setAudioDuration(audioDur);
            }

            m_originalF0 = m_currentFile->f0.values;

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
            emit fileLoaded(path);
            emit fileSelected(path);
        }

        bool PitchLabelerPage::saveFile() {
            if (!m_fileService->saveFile())
                return false;

            updateStatusInfo();
            m_fileListPanel->setFileSaved(m_currentFilePath);
            emit fileSaved(m_currentFilePath);
            emit modificationChanged(false);
            return true;
        }

        bool PitchLabelerPage::saveAllFiles() {
            if (m_fileService->hasUnsavedChanges()) {
                return saveFile();
            }
            return true;
        }

        void PitchLabelerPage::onUndo() {
            m_undoStack->undo();
            m_pianoRoll->update();
            updateUndoRedoState();
            emit modificationChanged(m_fileService->hasUnsavedChanges());
        }

        void PitchLabelerPage::onRedo() {
            m_undoStack->redo();
            m_pianoRoll->update();
            updateUndoRedoState();
            emit modificationChanged(m_fileService->hasUnsavedChanges());
        }

        void PitchLabelerPage::updateUndoRedoState() {
            m_actUndo->setEnabled(m_undoStack->canUndo());
            m_actRedo->setEnabled(m_undoStack->canRedo());
        }

        void PitchLabelerPage::onZoomIn() {
            m_pianoRoll->zoomIn();
            updateZoomStatus();
        }

        void PitchLabelerPage::onZoomOut() {
            m_pianoRoll->zoomOut();
            updateZoomStatus();
        }

        void PitchLabelerPage::onZoomReset() {
            m_pianoRoll->resetZoom();
            updateZoomStatus();
        }

        void PitchLabelerPage::updateZoomStatus() {
            int zoom = m_pianoRoll->getZoomPercent();
            emit zoomChanged(zoom);
        }

        void PitchLabelerPage::onPlayPause() {
            m_playWidget->setPlaying(!m_playWidget->isPlaying());
        }

        void PitchLabelerPage::onStop() {
            m_playWidget->setPlaying(false);
        }

        void PitchLabelerPage::updateTimeLabels(double sec) {
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

        void PitchLabelerPage::updatePlayheadPosition(double sec) {
            m_pianoRoll->setPlayheadTime(sec);
            updateTimeLabels(sec);

            // Don't fight the user — skip slider update while they're dragging
            if (m_playbackProgressSlider && m_currentFile &&
                !m_playbackProgressSlider->isSliderDown()) {
                double total = usToSec(m_currentFile->getTotalDuration());
                m_playbackProgressSlider->setRange(0, static_cast<int>(total * 1000));
                m_playbackProgressSlider->setValue(static_cast<int>(sec * 1000));
            }
        }

        void PitchLabelerPage::updatePlaybackState() {
            const bool playing = m_playWidget->isPlaying();
            m_actPlayPause->setIcon(QIcon(playing ? ":/icons/pause.svg" : ":/icons/play.svg"));
            m_actPlayPause->setToolTip(playing ? tr("Pause") : tr("Play"));
        }

        void PitchLabelerPage::setToolMode(ui::ToolMode mode) {
            m_pianoRoll->setToolMode(mode);
            m_btnToolSelect->setChecked(mode == ui::ToolSelect);
            m_btnToolModulation->setChecked(mode == ui::ToolModulation);
            m_btnToolDrift->setChecked(mode == ui::ToolDrift);
            emit toolModeChanged(static_cast<int>(mode));
        }

        void PitchLabelerPage::onNoteSelected(int noteIndex) {
            if (!m_currentFile)
                return;

            if (noteIndex >= 0 && noteIndex < static_cast<int>(m_currentFile->notes.size())) {
                m_propertyPanel->setSelectedNote(noteIndex);
            }
        }

        void PitchLabelerPage::onPositionClicked(double time, double midi) {
            Q_UNUSED(midi)
            emit positionChanged(time);
        }

        void PitchLabelerPage::onNextFile() {
            m_fileListPanel->selectNextFile();
        }

        void PitchLabelerPage::onPrevFile() {
            m_fileListPanel->selectPrevFile();
        }

        void PitchLabelerPage::updateStatusInfo() {
            if (!m_currentFilePath.isEmpty()) {
                emit fileStatusChanged(QFileInfo(m_currentFilePath).fileName());
            } else {
                emit fileStatusChanged(QString());
            }

            if (m_currentFile) {
                emit noteCountChanged(m_currentFile->getNoteCount());
            } else {
                emit noteCountChanged(0);
            }

            emit modificationChanged(hasUnsavedChanges());
        }

        void PitchLabelerPage::reloadCurrentFile() {
            if (!m_currentFilePath.isEmpty()) {
                m_fileService->reloadCurrentFile();
                m_currentFile = m_fileService->currentFile();
            }
        }

        void PitchLabelerPage::rebuildWindowShortcuts() {
            // Delete previous shortcuts
            qDeleteAll(m_windowShortcuts);
            m_windowShortcuts.clear();

            struct Entry {
                const dstools::SettingsKey<QString> &key;
                std::function<void()> action;
            };

            const Entry entries[] = {
                {PitchLabelerKeys::ToolSelect, [this]() { setToolMode(ui::ToolSelect); }},
                {PitchLabelerKeys::ToolModulation, [this]() { setToolMode(ui::ToolModulation); }},
                {PitchLabelerKeys::ToolDrift, [this]() { setToolMode(ui::ToolDrift); }},
                {PitchLabelerKeys::PlaybackPlayPause, [this]() { onPlayPause(); }},
                {PitchLabelerKeys::PlaybackStop, [this]() { onStop(); }},
                {PitchLabelerKeys::NavigationPrev, [this]() { onPrevFile(); }},
                {PitchLabelerKeys::NavigationNext, [this]() { onNextFile(); }},
            };

            for (const auto &[key, action] : entries) {
                const QKeySequence ks = m_settings.shortcut(key);
                if (!ks.isEmpty()) {
                    auto *sc = new QShortcut(ks, this, nullptr, nullptr, Qt::WindowShortcut);
                    connect(sc, &QShortcut::activated, this, action);
                    m_windowShortcuts.append(sc);
                }
            }
        }

    } // namespace pitchlabeler
} // namespace dstools
