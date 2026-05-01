/// @file IntervalTierView.h
/// @brief Interactive interval tier visualization and editing widget.

#pragma once

#include <QWidget>
#include <QPoint>
#include <QVector>

#include <dstools/ViewportController.h>
#include "TextGridDocument.h"
#include "BoundaryBindingManager.h"

class QPainter;
class QUndoStack;

namespace dstools {
namespace phonemelabeler {

using dstools::widgets::ViewportController;
using dstools::widgets::ViewportState;

/// @brief Renders a single TextGrid interval tier with boundary dragging,
///        interval selection, keyboard navigation, and binding-aware multi-tier editing.
class IntervalTierView : public QWidget {
    Q_OBJECT

public:
    /// @brief Constructs a view for one interval tier.
    /// @param tierIndex Index of the tier in the document.
    /// @param doc TextGrid document.
    /// @param undoStack Undo stack for edit commands.
    /// @param viewport Viewport controller for time-to-pixel mapping.
    /// @param bindingMgr Boundary binding manager for cross-tier linking.
    /// @param parent Optional parent widget.
    explicit IntervalTierView(int tierIndex, TextGridDocument *doc,
                               QUndoStack *undoStack, ViewportController *viewport,
                               BoundaryBindingManager *bindingMgr,
                               QWidget *parent = nullptr);
    ~IntervalTierView() override = default;

    /// @brief Updates the viewport state for rendering.
    /// @param state New viewport state.
    void setViewport(const ViewportState &state);

    /// @brief Returns the tier index this view represents.
    int tierIndex() const { return m_tierIndex; }

    /// @brief Sets whether this tier view is the active (focused) tier.
    /// @param active True to mark as active.
    void setActive(bool active);

    /// @brief Returns whether this tier view is active.
    [[nodiscard]] bool isActive() const { return m_active; }

    /// @brief Selects the next interval (keyboard navigation).
    void selectNextInterval();

    /// @brief Selects the previous interval (keyboard navigation).
    void selectPrevInterval();

    /// @brief Adjusts the selected boundary position by a time delta.
    /// @param deltaSec Time adjustment in seconds.
    void adjustSelectedBoundary(double deltaSec);

signals:
    /// @brief Emitted when an interval is clicked.
    /// @param intervalIndex Index of the clicked interval.
    /// @param startTime Start time of the interval.
    /// @param endTime End time of the interval.
    void intervalClicked(int intervalIndex, double startTime, double endTime);

    /// @brief Emitted when an interval is double-clicked.
    /// @param intervalIndex Index of the double-clicked interval.
    void intervalDoubleClicked(int intervalIndex);

    /// @brief Emitted when a boundary drag begins.
    /// @param boundaryIndex Index of the dragged boundary.
    void boundaryDragStarted(int boundaryIndex);

    /// @brief Emitted to request playback of an interval.
    /// @param startTime Playback start time.
    /// @param endTime Playback end time.
    void requestPlayback(double startTime, double endTime);

    /// @brief Emitted when this tier is activated by user interaction.
    /// @param tierIndex Index of the activated tier.
    void activated(int tierIndex);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    [[nodiscard]] int timeToX(double time) const;       ///< Converts time to pixel x.
    [[nodiscard]] double xToTime(int x) const;          ///< Converts pixel x to time.
    [[nodiscard]] int hitTestBoundary(int x) const;     ///< Returns boundary index at x, or -1.
    [[nodiscard]] int hitTestInterval(int x) const;     ///< Returns interval index at x, or -1.

    void drawIntervals(QPainter &painter);              ///< Draws interval backgrounds.
    void drawBoundaries(QPainter &painter);             ///< Draws boundary lines.
    void drawLabels(QPainter &painter);                 ///< Draws interval labels.
    void drawBindingLines(QPainter &painter);           ///< Draws cross-tier binding indicators.
    void drawSelection(QPainter &painter);              ///< Draws selection highlight.

    void startDrag(int boundaryIndex, double startTime);///< Begins boundary drag.
    void updateDrag(double currentTime);                ///< Updates drag position.
    void endDrag(double finalTime);                     ///< Commits boundary drag.

    int m_tierIndex;                                    ///< Tier index in the document.
    TextGridDocument *m_doc = nullptr;                  ///< Associated document.
    QUndoStack *m_undoStack = nullptr;                  ///< Undo stack for commands.
    ViewportController *m_viewport = nullptr;           ///< Viewport controller.
    BoundaryBindingManager *m_bindingMgr = nullptr;     ///< Binding manager.

    double m_viewStart = 0.0;                           ///< Visible range start in seconds.
    double m_viewEnd = 10.0;                            ///< Visible range end in seconds.
    double m_pixelsPerSecond = 200.0;                   ///< Current zoom level.

    /// @brief Interaction state machine.
    enum class State { Idle, Hovering, Dragging };
    State m_state = State::Idle;                        ///< Current interaction state.
    int m_hoveredBoundary = -1;                         ///< Hovered boundary index, or -1.
    int m_draggedBoundary = -1;                         ///< Dragged boundary index, or -1.
    int m_selectedInterval = -1;                        ///< Selected interval for keyboard nav.

    double m_dragStartTime = 0.0;                       ///< Time at drag start.
    std::vector<AlignedBoundary> m_dragAligned;          ///< Aligned boundaries during drag.
    std::vector<double> m_dragAlignedStartTimes;         ///< Original times of aligned boundaries.

    static constexpr int kBoundaryHitWidth = 6;         ///< Hit-test width in pixels.
    bool m_active = false;                              ///< Whether this tier is active.
};

} // namespace phonemelabeler
} // namespace dstools
