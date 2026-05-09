#pragma once

#include <QFrame>
#include <QScrollBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QVector>
#include <QSet>
#include <QUndoStack>

#include <dstools/PitchUtils.h>
#include <dstools/TimePos.h>
#include <dstools/ViewportController.h>

#include "PianoRollRenderer.h"
#include "PianoRollInputHandler.h"
#include "PitchProcessor.h"

#include <memory>
#include <set>
#include <map>

namespace dstools {
class AppSettings;

namespace pitchlabeler {

class DSFile;

namespace ui {

using dstools::NotePitch;
using dstools::parseNoteName;
using dstools::freqToMidi;
using dstools::midiToFreq;
using dstools::midiToNoteName;
using dstools::midiToNoteString;
using dstools::shiftNoteCents;

/// Piano roll view for editing notes and F0
class PianoRollView : public QFrame {
    Q_OBJECT

public:
    explicit PianoRollView(QWidget *parent = nullptr);
    ~PianoRollView() override;

    void setDSFile(std::shared_ptr<DSFile> ds);
    void setUndoStack(QUndoStack *stack) { m_undoStack = stack; }
    void clear();

    /// Set audio file duration (seconds) to limit piano roll max length
    void setAudioDuration(double sec);

    /// Set shared viewport controller for horizontal zoom/scroll synchronization
    void setViewportController(dstools::widgets::ViewportController *vc);

    // Zoom controls
    void zoomIn();
    void zoomOut();
    void resetZoom();
    int getZoomPercent() const;

    // Playhead
    void setPlayheadTime(double sec);
    void setPlayheadState(bool playing);

    // Tool mode
    void setToolMode(ToolMode mode);
    ToolMode toolMode() const { return m_toolMode; }

    // A/B comparison
    void setABComparisonActive(bool active);
    bool isABComparisonActive() const { return m_abComparisonActive; }
    void storeOriginalF0(const std::vector<int32_t> &original);

    // Config persistence (following SlurCutter F0Widget pattern)
    void loadConfig(dstools::AppSettings &settings);
    void pullConfig(dstools::AppSettings &settings) const;

signals:
    void noteSelected(int noteIndex);
    void selectionChanged(const std::set<int> &selected);
    void positionClicked(double time, double midi);
    void rulerClicked(double timeSec);
    void fileEdited();
    void toolModeChanged(int mode);
    void noteGlideChanged(int noteIndex, const QString &glide);
    void noteSlurToggled(int noteIndex);
    void noteRestToggled(int noteIndex);
    void noteMergeLeft(int noteIndex);
    void noteDeleteRequested(const std::vector<int> &indices);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    // Composed components
    PianoRollInputHandler m_inputHandler;

    // Viewport controller (shared horizontal zoom/scroll)
    dstools::widgets::ViewportController *m_viewport = nullptr;
    bool m_syncingViewport = false;
    void onViewportChanged(const dstools::widgets::ViewportState &state);
    void syncToViewport(double startSec, double endSec);

    // Data
    std::shared_ptr<DSFile> m_dsFile;
    QUndoStack *m_undoStack = nullptr;
    double m_audioDuration = 0.0;

    // Display parameters
    double m_hScale = 100.0;
    double m_vScale = 20.0;

    // Scroll bars
    QScrollBar *m_hScrollBar = nullptr;
    QScrollBar *m_vScrollBar = nullptr;

    // Viewport offset
    double m_scrollX = 0.0;
    double m_scrollY = 0.0;

    // Playhead
    double m_playheadPos = 0.0;
    bool m_isPlaying = false;

    // Tool mode
    ToolMode m_toolMode = ToolSelect;

    // A/B comparison
    bool m_abComparisonActive = false;
    std::vector<int32_t> m_originalF0;

    // Display options
    bool m_showPitchTextOverlay = false;
    bool m_showPhonemeTexts = true;
    bool m_showCrosshairAndPitch = true;
    bool m_snapToKey = false;

    // Context note for right-click menu
    int m_contextNoteIndex = -1;

    // Multi-selection
    std::set<int> m_selectedNotes;

    // Layout constants
    static constexpr int PianoWidth = 52;
    static constexpr int RulerHeight = 24;
    static constexpr int ScrollBarSize = 14;
    static constexpr int MinMidi = 24;
    static constexpr int MaxMidi = 96;

    // Coordinate conversion
    double timeToX(double time) const;
    double xToTime(double x) const;
    double midiToY(double midi) const;
    double yToMidi(double y) const;
    int sceneXToWidget(double sceneX) const;
    int sceneYToWidget(double sceneY) const;
    double widgetXToScene(int wx) const;
    double widgetYToScene(int wy) const;

    void updateScrollBars();

    // Note helpers
    int getNoteAtPosition(int x, int y) const;

    // Selection helpers
    void selectNotes(const std::set<int> &indices);
    void selectAllNotes();
    void clearSelection();

    // Pitch editing
    void doPitchMove(const std::vector<int> &indices, int deltaCents);

    // Build RenderState snapshot for renderer
    RenderState buildRenderState() const;

    // Setup input handler callbacks
    void setupInputCallbacks();

    // Restore cursor to tool-mode default
    void restoreToolCursor();

    // Context menus
    QMenu *m_bgMenu = nullptr;
    QMenu *m_noteMenu = nullptr;
    QAction *m_actShowPitchTextOverlay = nullptr;
    QAction *m_actShowPhonemeTexts = nullptr;
    QAction *m_actShowCrosshairAndPitch = nullptr;
    QAction *m_actSnapToKey = nullptr;

    QAction *m_actMergeLeft = nullptr;
    QAction *m_actToggleRest = nullptr;
    QAction *m_actToggleSlur = nullptr;
    QActionGroup *m_actGlideGroup = nullptr;
    QAction *m_actGlideNone = nullptr;
    QAction *m_actGlideUp = nullptr;
    QAction *m_actGlideDown = nullptr;

    void buildContextMenu();
    void buildNoteMenu();
    void updateNoteMenuState();
};

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
