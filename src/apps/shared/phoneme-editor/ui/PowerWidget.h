/// @file PowerWidget.h
/// @brief Audio power (RMS energy) visualization widget.

#pragma once

#include <QWidget>
#include <vector>
#include <QPoint>

#include <dstools/ViewportController.h>
#include <dstools/TimePos.h>
#include <dstools/PlayWidget.h>
#include "BoundaryBindingManager.h"

class QPainter;
class QUndoStack;

namespace dstools {
namespace phonemelabeler {

using dstools::widgets::ViewportController;
using dstools::widgets::ViewportState;

class TextGridDocument;

/// @brief Displays audio power curve synchronized with the viewport, with boundary
///        overlay and dragging support.
class PowerWidget : public QWidget {
    Q_OBJECT

public:
    /// @brief Constructs the power widget.
    /// @param viewport Viewport controller for time synchronization.
    /// @param parent Optional parent widget.
    explicit PowerWidget(ViewportController *viewport, QWidget *parent = nullptr);
    ~PowerWidget() override;

    /// @brief Sets the audio sample data.
    /// @param samples Audio samples.
    /// @param sampleRate Sample rate in Hz.
    void setAudioData(const std::vector<float> &samples, int sampleRate);

    /// @brief Updates the viewport state.
    /// @param state New viewport state.
    void setViewport(const ViewportState &state);

    /// @brief Sets the TextGrid document for boundary display.
    void setDocument(TextGridDocument *doc) { m_document = doc; }

    /// @brief Sets the boundary binding manager.
    void setBindingManager(BoundaryBindingManager *mgr) { m_bindingMgr = mgr; }

    /// @brief Sets the undo stack for boundary edit commands.
    void setUndoStack(QUndoStack *stack) { m_undoStack = stack; }

    /// @brief Sets the play widget for audio playback integration.
    void setPlayWidget(dstools::widgets::PlayWidget *pw) { m_playWidget = pw; }

    /// @brief Triggers a repaint of the boundary overlay.
    void updateBoundaryOverlay();

signals:
    void boundaryDragStarted(int tierIndex, int boundaryIndex);  ///< Boundary drag began.
    void boundaryDragging(int tierIndex, int boundaryIndex, TimePos newTime); ///< Boundary being dragged.
    void boundaryDragFinished(int tierIndex, int boundaryIndex, TimePos newTime); ///< Boundary drag ended.
    void hoveredBoundaryChanged(int boundaryIndex); ///< Hovered boundary changed.
    void entryScrollRequested(int delta);           ///< Scroll request from wheel event.

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void rebuildPowerCache();                           ///< Recomputes cached power values.
    void drawPower(QPainter &painter);                  ///< Draws the power curve.
    void drawReferenceLines(QPainter &painter);         ///< Draws dB reference lines.
    void drawBoundaryOverlay(QPainter &painter);        ///< Draws boundary lines.

    [[nodiscard]] int hitTestBoundary(int x) const;     ///< Returns boundary index at x, or -1.
    void startBoundaryDrag(int boundaryIndex, TimePos time);   ///< Begins boundary drag.
    void updateBoundaryDrag(TimePos currentTime);              ///< Updates drag position.
    void endBoundaryDrag(TimePos finalTime);                   ///< Commits boundary drag.
    void findSurroundingBoundaries(double time, double &start, double &end) const;

    [[nodiscard]] double xToTime(int x) const;          ///< Converts pixel x to time.
    [[nodiscard]] int timeToX(double time) const;       ///< Converts time to pixel x.

    ViewportController *m_viewport = nullptr;           ///< Viewport controller.
    TextGridDocument *m_document = nullptr;             ///< Associated document.
    BoundaryBindingManager *m_bindingMgr = nullptr;     ///< Binding manager.
    QUndoStack *m_undoStack = nullptr;                  ///< Undo stack.
    dstools::widgets::PlayWidget *m_playWidget = nullptr;

    std::vector<float> m_samples;                       ///< Raw audio samples.
    int m_sampleRate = 44100;                           ///< Audio sample rate in Hz.

    std::vector<float> m_powerCache;                    ///< Power dB values per pixel column.

    double m_viewStart = 0.0;                           ///< Visible range start in seconds.
    double m_viewEnd = 10.0;                            ///< Visible range end in seconds.
    double m_pixelsPerSecond = 200.0;                   ///< Current zoom level.

    static constexpr int kUnitSize = 60;                ///< RMS unit size in samples.
    static constexpr int kWindowSize = 300;             ///< RMS window size in samples.
    static constexpr float kMinPower = -48.0f;          ///< Minimum displayed power in dB.
    static constexpr float kMaxPower = 0.0f;            ///< Maximum displayed power in dB.
    static constexpr float kRefValue = 32768.0f;        ///< Reference value (2^15).

    bool m_dragging = false;                            ///< Whether viewport is being scrolled.
    QPoint m_dragStartPos;                              ///< Mouse position at drag start.
    double m_dragStartTime = 0.0;                       ///< View start time at drag start.

    bool m_boundaryDragging = false;                    ///< Whether a boundary is being dragged.
    int m_draggedBoundary = -1;                         ///< Index of dragged boundary.
    int m_draggedTier = -1;                             ///< Tier of dragged boundary.
    TimePos m_boundaryDragStartTime = 0;               ///< Original time of dragged boundary.
    std::vector<AlignedBoundary> m_dragAligned;          ///< Aligned boundaries during drag.
    std::vector<TimePos> m_dragAlignedStartTimes;         ///< Original times of aligned boundaries.

    static constexpr int kBoundaryHitWidth = 8;         ///< Hit-test width in pixels.
    int m_hoveredBoundary = -1;                         ///< Hovered boundary index, or -1.
};

} // namespace phonemelabeler
} // namespace dstools
