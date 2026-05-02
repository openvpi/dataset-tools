#pragma once

#include <QPoint>
#include <QRect>
#include <Qt>

#include <dstools/PitchUtils.h>
#include <dstools/TimePos.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <map>
#include <vector>

class QMouseEvent;
class QWheelEvent;
class QKeyEvent;
class QUndoStack;

namespace dstools {
namespace pitchlabeler {

class DSFile;

namespace ui {

enum ToolMode {
    ToolSelect = 0,
    ToolModulation = 1,
    ToolDrift = 2,
};

struct RenderState;

/// Callback interface from InputHandler back to PianoRollView.
struct InputHandlerCallbacks {
    // Coordinate conversions (use RenderState inline versions via lambda)
    std::function<double(int)> widgetXToScene;
    std::function<double(int)> widgetYToScene;
    std::function<double(double)> xToTime;
    std::function<double(double)> yToMidi;
    std::function<double(double)> timeToX;
    std::function<double(double)> midiToY;
    std::function<int(double)> sceneXToWidget;
    std::function<int(double)> sceneYToWidget;

    // Actions
    std::function<void()> update;
    std::function<void(Qt::CursorShape)> setCursor;
    std::function<void(const std::set<int> &)> selectNotes;
    std::function<void()> selectAllNotes;
    std::function<void()> clearSelection;
    std::function<void(const std::vector<int> &, int)> doPitchMove;
    std::function<void()> restoreToolCursor;
    std::function<int(int, int)> getNoteAtPosition;
    std::function<double(int)> getRestMidi;

    // Viewport
    std::function<void(double, double)> viewportZoomAt;
    std::function<void(double)> viewportScrollBy;
    std::function<double()> viewportViewCenter;
    std::function<void(double)> viewportSetPPS;

    // Scroll bars
    std::function<void(int)> setVScrollValue;
    std::function<int()> getVScrollValue;

    // Signals (emit through PianoRollView)
    std::function<void(int)> emitNoteSelected;
    std::function<void(double, double)> emitPositionClicked;
    std::function<void(double)> emitRulerClicked;
    std::function<void()> emitFileEdited;
    std::function<void(const std::vector<int> &)> emitNoteDeleteRequested;
};

/// Handles all mouse and keyboard input for the piano roll.
/// Contains drag state machines. Communicates via callbacks.
class PianoRollInputHandler {
public:
    explicit PianoRollInputHandler();

    void setCallbacks(const InputHandlerCallbacks &cb) { m_cb = cb; }

    // Event handlers — return true if event was handled
    void handleMousePress(QMouseEvent *event, ToolMode toolMode,
                          const std::shared_ptr<DSFile> &dsFile,
                          std::set<int> &selectedNotes);
    void handleMouseMove(QMouseEvent *event, ToolMode toolMode,
                         const std::shared_ptr<DSFile> &dsFile,
                         const std::set<int> &selectedNotes,
                         bool showCrosshairAndPitch);
    void handleMouseRelease(QMouseEvent *event, ToolMode toolMode,
                            const std::shared_ptr<DSFile> &dsFile,
                            std::set<int> &selectedNotes,
                            QUndoStack *undoStack);
    void handleMouseDoubleClick(QMouseEvent *event,
                                const std::shared_ptr<DSFile> &dsFile,
                                std::set<int> &selectedNotes,
                                QUndoStack *undoStack);
    void handleWheel(QWheelEvent *event);
    void handleKeyPress(QKeyEvent *event,
                        const std::shared_ptr<DSFile> &dsFile,
                        const std::set<int> &selectedNotes);

    // State accessors for renderer
    bool isDragging() const { return m_isDragging; }
    bool isDraggingNote() const { return m_draggingNote; }
    int dragAccumulatedCents() const { return m_dragAccumulatedCents; }
    const std::map<int, double> &dragOrigMidi() const { return m_dragOrigMidi; }
    bool isModulationDragging() const { return m_modulationDragging; }
    bool isDriftDragging() const { return m_driftDragging; }
    double modulationDragAmount() const { return m_modulationDragAmount; }
    double driftDragAmount() const { return m_driftDragAmount; }
    const std::vector<int32_t> &preAdjustF0() const { return m_preAdjustF0; }
    bool isRubberBandActive() const { return m_rubberBandActive; }
    QRect rubberBandRect() const { return m_rubberBandRect; }
    bool isRulerDragging() const { return m_rulerDragging; }
    QPoint mousePos() const { return m_mousePos; }

    // Reset all drag state (called on setDSFile / clear)
    void reset();

    // Layout constants (shared with View)
    static constexpr int PianoWidth = 52;
    static constexpr int RulerHeight = 24;
    static constexpr double ModulationDragSensitivity = 80.0;

private:
    InputHandlerCallbacks m_cb;

    // Mouse state
    QPoint m_dragStart;
    QPoint m_mousePos;
    bool m_isDragging = false;
    Qt::MouseButton m_dragButton = Qt::NoButton;

    // Note pitch drag state (SELECT mode)
    bool m_draggingNote = false;
    double m_dragStartMidi = 0.0;
    int m_dragAccumulatedCents = 0;
    std::map<int, double> m_dragOrigMidi;

    // Modulation/Drift drag state
    bool m_modulationDragging = false;
    bool m_driftDragging = false;
    double m_modulationDragStartY = 0.0;
    double m_driftDragStartY = 0.0;
    double m_modulationDragAmount = 1.0;
    double m_driftDragAmount = 1.0;
    std::vector<int32_t> m_preAdjustF0;

    // Rubber-band selection
    bool m_rubberBandActive = false;
    QPoint m_rubberBandStart;
    QRect m_rubberBandRect;

    // Ruler scrub
    bool m_rulerDragging = false;
};

} // namespace ui
} // namespace pitchlabeler
} // namespace dstools
