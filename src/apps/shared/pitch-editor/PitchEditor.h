/// @file PitchEditor.h
/// @brief Reusable F0/MIDI pitch editor widget extracted from PitchLabelerPage.

#pragma once

#include <QAction>
#include <QActionGroup>
#include <QLabel>
#include <QProxyStyle>
#include <QShortcut>
#include <QSlider>
#include <QSplitter>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>
#include <QUndoStack>
#include <QWidget>

#include <dstools/PlayWidget.h>
#include <dstools/TimePos.h>
#include <dstools/ViewportController.h>

#include "ui/PianoRollView.h"

#include <memory>
#include <vector>

namespace dstools {
namespace pitchlabeler {

class DSFile;

namespace ui {
class PianoRollView;
class PropertyPanel;
}

/// @brief Core pitch editing widget: PianoRollView + PropertyPanel + toolbar + playback.
///
/// Contains no file I/O, no FileListPanel, no IPageActions, no settings persistence.
/// PitchLabelerPage (and future DsPitchLabelerPage) compose this widget.
class PitchEditor : public QWidget {
    Q_OBJECT

public:
    explicit PitchEditor(QWidget *parent = nullptr);
    ~PitchEditor() override;

    /// Load a DSFile for editing in the piano roll.
    void loadDSFile(std::shared_ptr<DSFile> file);

    /// Load audio for playback.
    void loadAudio(const QString &path, double duration);

    /// Clear the editor to empty state.
    void clear();

    /// Access the undo stack.
    [[nodiscard]] QUndoStack *undoStack() const { return m_undoStack; }

    /// Access the viewport controller.
    [[nodiscard]] dstools::widgets::ViewportController *viewport() const { return m_viewport; }

    /// Access the play widget.
    [[nodiscard]] dstools::widgets::PlayWidget *playWidget() const { return m_playWidget; }

    /// Access the toolbar (for embedding page-level actions).
    [[nodiscard]] QToolBar *toolbar() const { return m_toolbar; }

    /// Access the piano roll view.
    [[nodiscard]] ui::PianoRollView *pianoRoll() const { return m_pianoRoll; }

    /// Access the current DSFile.
    [[nodiscard]] std::shared_ptr<DSFile> currentFile() const { return m_currentFile; }

    // --- Actions for menu/shortcut binding ---
    [[nodiscard]] QAction *saveAction() const { return m_actSave; }
    [[nodiscard]] QAction *saveAllAction() const { return m_actSaveAll; }
    [[nodiscard]] QAction *undoAction() const { return m_actUndo; }
    [[nodiscard]] QAction *redoAction() const { return m_actRedo; }
    [[nodiscard]] QAction *zoomInAction() const { return m_actZoomIn; }
    [[nodiscard]] QAction *zoomOutAction() const { return m_actZoomOut; }
    [[nodiscard]] QAction *zoomResetAction() const { return m_actZoomReset; }
    [[nodiscard]] QAction *abCompareAction() const { return m_actABCompare; }
    [[nodiscard]] QAction *playPauseAction() const { return m_actPlayPause; }
    [[nodiscard]] QAction *stopAction() const { return m_actStop; }

    // --- A/B comparison ---
    void setOriginalF0(const std::vector<int32_t> &f0);
    void setABComparisonActive(bool active);

    // --- Configuration ---
    void loadConfig(dstools::AppSettings &settings);
    void pullConfig(dstools::AppSettings &settings);

    QByteArray saveSplitterState() const;
    void restoreSplitterState(const QByteArray &state);

    // --- Tool mode shortcuts ---
    void rebuildWindowShortcuts(dstools::AppSettings &settings);

signals:
    void fileEdited();
    void positionChanged(double sec);
    void toolModeChanged(int mode);
    void noteSelected(int index);
    void zoomChanged(int percent);
    void noteCountChanged(int count);
    void modificationChanged(bool modified);
    void noteDeleteRequested(const std::vector<int> &indices);
    void noteGlideChanged(int idx, const QString &glide);
    void noteSlurToggled(int idx);
    void noteRestToggled(int idx);
    void noteMergeLeft(int idx);

private:
    // Core state
    std::shared_ptr<DSFile> m_currentFile;
    QUndoStack *m_undoStack = nullptr;
    dstools::widgets::ViewportController *m_viewport = nullptr;
    dstools::widgets::PlayWidget *m_playWidget = nullptr;

    // UI Components
    QToolBar *m_toolbar = nullptr;
    QStackedWidget *m_mainStack = nullptr;
    QWidget *m_emptyPage = nullptr;
    QSplitter *m_rightSplitter = nullptr;

    ui::PianoRollView *m_pianoRoll = nullptr;
    ui::PropertyPanel *m_propertyPanel = nullptr;

    // Playback progress
    QWidget *m_playbackProgressWidget = nullptr;
    QSlider *m_playbackProgressSlider = nullptr;
    QLabel *m_progressCurrentTime = nullptr;
    QLabel *m_progressTotalTime = nullptr;

    // Tool mode buttons
    QToolButton *m_btnToolSelect = nullptr;
    QToolButton *m_btnToolModulation = nullptr;
    QToolButton *m_btnToolDrift = nullptr;
    QToolButton *m_btnToolAudition = nullptr;
    QActionGroup *m_toolModeGroup = nullptr;

    // Waveform / volume controls
    QToolButton *m_btnWaveformToggle = nullptr;
    QSlider *m_volumeSlider = nullptr;
    QLabel *m_volumeLabel = nullptr;

    // Actions - Edit
    QAction *m_actSave = nullptr;
    QAction *m_actSaveAll = nullptr;
    QAction *m_actUndo = nullptr;
    QAction *m_actRedo = nullptr;

    // Actions - View
    QAction *m_actZoomIn = nullptr;
    QAction *m_actZoomOut = nullptr;
    QAction *m_actZoomReset = nullptr;

    // Actions - Tools
    QAction *m_actABCompare = nullptr;

    // Toolbar actions
    QAction *m_actPlayPause = nullptr;
    QAction *m_actStop = nullptr;

    // A/B comparison
    bool m_abComparisonActive = false;

    // Shortcuts
    QList<QShortcut *> m_windowShortcuts;

    // Helpers
    void buildActions();
    void buildLayout();
    void connectSignals();

    // Edit
    void onUndo();
    void onRedo();
    void updateUndoRedoState();

    // View
    void onZoomIn();
    void onZoomOut();
    void onZoomReset();
    void updateZoomStatus();

    // Playback
    void onPlayPause();
    void onStop();
    void updatePlayheadPosition(double sec);
    void updatePlaybackState();
    void updateTimeLabels(double sec);

    // Tool mode
    void setToolMode(ui::ToolMode mode);

    // Selection
    void onNoteSelected(int noteIndex);
    void onPositionClicked(double time, double midi);

    // Status
    void updateStatusInfo();
};

} // namespace pitchlabeler
} // namespace dstools
