/// @file PitchEditor.h
/// @brief Reusable F0/MIDI pitch editor widget extracted from PitchLabelerPage.

#pragma once

#include "../audio-visualizer/EditorContainerBase.h"
#include "ui/PianoRollChartPanel.h"
#include "ui/PropertyPanel.h"

#include <QAction>
#include <QActionGroup>
#include <QLabel>
#include <QProxyStyle>
#include <QSplitter>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>
#include <QUndoStack>
#include <QWidget>
#include <dsfw/widgets/PlayWidget.h>
#include <dsfw/TimePos.h>
#include <dsfw/widgets/ViewportController.h>
#include <memory>
#include <vector>
#include <dstools/DsPitchDocument.h>

namespace dstools {
namespace pitchlabeler {

namespace ui {
class PianoRollChartPanel;
class PropertyPanel;
class NoteBoundaryModel;
class NoteTierLabel;
} // namespace ui

/// @brief Core pitch editing widget: PianoRollChartPanel + PropertyPanel + toolbar + playback.
///
/// Contains no file I/O, no FileListPanel, no IPageActions, no settings persistence.
/// PitchLabelerPage (and future DsPitchLabelerPage) compose this widget.
class PitchEditor : public EditorContainerBase {
    Q_OBJECT

public:
    explicit PitchEditor(QWidget* parent = nullptr);
    ~PitchEditor() override;

    /// Load a DsPitchDocument for editing in the piano roll.
    void loadDsPitchDocument(std::shared_ptr<DsPitchDocument> file);

    /// Load audio for playback.
    void loadAudio(const QString& path, double duration);

    /// Clear the editor to empty state.
    void clear();

    /// Access the viewport controller.
    [[nodiscard]] dsfw::widgets::ViewportController* viewport() const { return m_viewport; }

    /// Access the play widget.
    [[nodiscard]] dsfw::widgets::PlayWidget* playWidget() const { return m_playWidget; }

    /// Access the toolbar (for embedding page-level actions).
    [[nodiscard]] QToolBar* toolbar() const { return m_toolbar; }

    /// Access the piano roll chart panel.
    [[nodiscard]] ui::PianoRollChartPanel* pianoRollChart() const { return m_pianoRollChart; }

    /// Access the current DsPitchDocument.
    [[nodiscard]] std::shared_ptr<DsPitchDocument> currentFile() const { return m_currentFile; }

    // --- Actions for menu/shortcut binding ---
    [[nodiscard]] QAction* saveAction() const { return m_actSave; }
    [[nodiscard]] QAction* saveAllAction() const { return m_actSaveAll; }
    [[nodiscard]] QAction* zoomInAction() const { return m_actZoomIn; }
    [[nodiscard]] QAction* zoomOutAction() const { return m_actZoomOut; }
    [[nodiscard]] QAction* zoomResetAction() const { return m_actZoomReset; }
    [[nodiscard]] QAction* abCompareAction() const { return m_actABCompare; }

    // --- A/B comparison ---
    void setABComparisonActive(bool active);

    // --- Configuration ---
    void loadConfig(dstools::AppSettings& settings);
    void pullConfig(dstools::AppSettings& settings);

signals:
    void fileEdited();
    void positionChanged(double sec);
    void toolModeChanged(int mode);
    void noteSelected(int index);
    void zoomChanged(int percent);
    void noteCountChanged(int count);
    void noteDeleteRequested(const std::vector<int>& indices);
    void noteGlideChanged(int idx, const QString& glide);
    void noteSlurToggled(int idx);
    void noteRestToggled(int idx);
    void noteMergeLeft(int idx);

private:
    // Core state
    std::shared_ptr<DsPitchDocument> m_currentFile;

    // UI Components
    QToolBar* m_toolbar = nullptr;
    QStackedWidget* m_mainStack = nullptr;
    QWidget* m_emptyPage = nullptr;

    ui::PianoRollChartPanel* m_pianoRollChart = nullptr;
    ui::PropertyPanel* m_propertyPanel = nullptr;

    // Playback progress is now handled by the container's PlayCursorOverlay
    // Old playback widgets (m_playbackProgressSlider, m_progressCurrentTime, m_progressTotalTime) are removed

    // Tool mode buttons
    QToolButton* m_btnToolSelect = nullptr;
    QToolButton* m_btnToolModulation = nullptr;
    QToolButton* m_btnToolDrift = nullptr;
    QToolButton* m_btnToolAudition = nullptr;
    QActionGroup* m_toolModeGroup = nullptr;

    // Volume controls
    QSlider* m_volumeSlider = nullptr;
    QLabel* m_volumeLabel = nullptr;

    // Actions - Edit
    QAction* m_actSave = nullptr;
    QAction* m_actSaveAll = nullptr;

    // Actions - View
    QAction* m_actZoomIn = nullptr;
    QAction* m_actZoomOut = nullptr;
    QAction* m_actZoomReset = nullptr;

    // Actions - Tools
    QAction* m_actABCompare = nullptr;

    // A/B comparison
    bool m_abComparisonActive = false;

    QList<int> m_undoDataIndices;

    // Helpers
    void buildActions();
    void buildLayout();
    void connectSignals();

    // Edit
    void onUndo();
    void onRedo();

    // View
    void onZoomIn();
    void onZoomOut();
    void onZoomReset();
    void updateZoomStatus();

    // Playback
    void updatePlayheadPosition(double sec);
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
