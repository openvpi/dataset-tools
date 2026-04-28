#pragma once

#include <QFrame>
#include <QScrollBar>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QVector>
#include <QSet>

#include <memory>
#include <set>
#include <map>

namespace dstools {
class AppSettings;

namespace pitchlabeler {

class DSFile;

namespace ui {

/// Tool modes for pitch editing (matches Python reference ToolMode)
enum ToolMode {
    ToolSelect = 0,
    ToolModulation = 1,
    ToolDrift = 2,
};

/// Parsed representation of a DS note string (e.g. "C#4+12")
struct NotePitch {
    QString name;       // e.g. "C#"
    int octave = 0;     // e.g. 4
    int cents = 0;      // e.g. 12, always in [-50, +50]
    double midiNumber = 0.0; // full precision MIDI number
    bool valid = false;
};

/// Parse a DS note string like "C#4+12", "Bb3", "rest" into NotePitch.
/// Returns invalid NotePitch for "rest" or unparseable strings.
NotePitch parseNoteName(const QString &noteStr);

/// Convert frequency in Hz to MIDI note number (float).
/// Returns 0 for freq <= 0.
double freqToMidi(double freq);

/// Convert MIDI note number to frequency in Hz.
double midiToFreq(double midi);

/// Convert MIDI note number to display name (e.g. "C4", "C#4")
QString midiToNoteName(int midiNote);

/// Shift a note name string by delta_cents, with auto carry-over at ±50.
/// Example: shiftNoteCents("C4+48", 5) -> "C#4+3"
QString shiftNoteCents(const QString &noteStr, int deltaCents);

/// Convert a fractional MIDI number to a DS note string (auto carry-over).
QString midiToNoteString(double midiFloat);

/// Piano roll view for editing notes and F0
class PianoRollView : public QFrame {
    Q_OBJECT

public:
    explicit PianoRollView(QWidget *parent = nullptr);
    ~PianoRollView() override;

    void setDSFile(std::shared_ptr<DSFile> ds);
    void clear();

    /// Set audio file duration (seconds) to limit piano roll max length
    void setAudioDuration(double sec);

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
    void storeOriginalF0(const std::vector<double> &original);

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
    // Data
    std::shared_ptr<DSFile> m_dsFile;
    double m_audioDuration = 0.0;  // audio file duration in seconds (0 = no limit)

    // Display parameters
    double m_hScale = 100.0;        // pixels per second (horizontal zoom)
    double m_vScale = 20.0;         // pixels per semitone (vertical zoom)

    // Scroll bars
    QScrollBar *m_hScrollBar = nullptr;
    QScrollBar *m_vScrollBar = nullptr;

    // Viewport offset (scroll position in pixels)
    double m_scrollX = 0.0;         // horizontal scroll offset in pixels
    double m_scrollY = 0.0;         // vertical scroll offset in pixels

    // Playhead
    double m_playheadPos = 0.0;
    bool m_isPlaying = false;

    // Tool mode
    ToolMode m_toolMode = ToolSelect;

    // A/B comparison
    bool m_abComparisonActive = false;
    std::vector<double> m_originalF0;

    // Display options (matching SlurCutter pattern)
    bool m_showPitchTextOverlay = false;
    bool m_showPhonemeTexts = true;
    bool m_showCrosshairAndPitch = true;
    bool m_snapToKey = false;

    // Mouse state
    QPoint m_dragStart;
    QPoint m_mousePos;              // for crosshair tracking
    bool m_isDragging = false;
    Qt::MouseButton m_dragButton = Qt::NoButton;
    int m_contextNoteIndex = -1;    // note under right-click

    // Multi-selection (matching Python reference: set of note indices)
    std::set<int> m_selectedNotes;

    // Note pitch drag state (SELECT mode)
    bool m_draggingNote = false;
    double m_dragStartMidi = 0.0;
    int m_dragAccumulatedCents = 0;
    std::map<int, double> m_dragOrigMidi;  // original MIDI per selected note

    // Modulation/Drift drag state
    bool m_modulationDragging = false;
    bool m_driftDragging = false;
    double m_modulationDragStartY = 0.0;
    double m_driftDragStartY = 0.0;
    double m_modulationDragAmount = 1.0;
    double m_driftDragAmount = 1.0;
    std::vector<double> m_preAdjustF0;  // snapshot before drag

    // Rubber-band selection
    bool m_rubberBandActive = false;
    QPoint m_rubberBandStart;
    QRect m_rubberBandRect;

    // Ruler scrub
    bool m_rulerDragging = false;

    static constexpr double ModulationDragSensitivity = 80.0;

    // Layout constants
    static constexpr int PianoWidth = 52;
    static constexpr int RulerHeight = 24;
    static constexpr int ScrollBarSize = 14;
    static constexpr int MinMidi = 24;   // C2
    static constexpr int MaxMidi = 96;   // C7

    // Coordinate conversion (scene coordinates)
    double timeToX(double time) const;
    double xToTime(double x) const;
    double midiToY(double midi) const;
    double yToMidi(double y) const;

    // Scene to widget coordinates (accounting for scroll)
    int sceneXToWidget(double sceneX) const;
    int sceneYToWidget(double sceneY) const;
    double widgetXToScene(int wx) const;
    double widgetYToScene(int wy) const;

    void updateScrollBars();

    // Drawing helpers
    void drawGrid(QPainter &p, int w, int h);
    void drawPianoKeys(QPainter &p, int h);
    void drawRuler(QPainter &p, int w);
    void drawNotes(QPainter &p, int w, int h);
    void drawF0Curve(QPainter &p, int w, int h);
    void drawPlayhead(QPainter &p, int w, int h);
    void drawCrosshair(QPainter &p, int w, int h);

    // Get rest note MIDI from nearest non-rest neighbor
    double getRestMidi(int index) const;

    // Note helpers
    int getNoteAtPosition(int x, int y) const;

    // Selection helpers
    void selectNotes(const std::set<int> &indices);
    void selectAllNotes();
    void clearSelection();

    // Pitch editing helpers
    void doPitchMove(const std::vector<int> &indices, int deltaCents);
    void applyModulationDriftPreview();
    static std::vector<double> movingAverage(const std::vector<double> &values, int window);

    // Drawing helpers for selection
    void drawRubberBand(QPainter &p);
    void drawSnapGuide(QPainter &p, int w, int h);

    // Context menus
    QMenu *m_bgMenu = nullptr;
    QMenu *m_noteMenu = nullptr;
    QAction *m_actShowPitchTextOverlay = nullptr;
    QAction *m_actShowPhonemeTexts = nullptr;
    QAction *m_actShowCrosshairAndPitch = nullptr;
    QAction *m_actSnapToKey = nullptr;

    // Note menu actions
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

    // Restore cursor to the tool-mode default (ArrowCursor for Select, SizeVerCursor for Modulation/Drift)
    void restoreToolCursor();
};

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
