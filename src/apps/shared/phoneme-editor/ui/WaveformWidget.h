/// @file WaveformWidget.h
/// @brief Audio waveform visualization widget with playback cursor and boundary editing.

#pragma once

#include <QWidget>
#include <vector>
#include <QPoint>

#include <dstools/ViewportController.h>
#include <dstools/TimePos.h>
#include "IBoundaryModel.h"

#include <dstools/PlayWidget.h>

class QPainter;
class QPixmap;
class QUndoStack;

namespace dstools {
namespace phonemelabeler {

using dstools::widgets::ViewportController;
using dstools::widgets::ViewportState;

class BoundaryDragController;

/// @brief Renders audio waveform with min/max caching, playback cursor, boundary overlay,
///        and drag-based boundary editing.
class WaveformWidget : public QWidget {
    Q_OBJECT

public:
    /// @brief Constructs the waveform widget.
    /// @param viewport Viewport controller for time synchronization.
    /// @param parent Optional parent widget.
    explicit WaveformWidget(ViewportController *viewport, QWidget *parent = nullptr);
    ~WaveformWidget() override;

    /// @brief Sets the audio sample data.
    /// @param samples Audio samples.
    /// @param sampleRate Sample rate in Hz.
    void setAudioData(const std::vector<float> &samples, int sampleRate);

    /// @brief Loads audio from a file path.
    /// @param path Path to the audio file.
    void loadAudio(const QString &path);

    /// @brief Sets the boundary model for boundary display and editing.
    /// @param model Boundary model (IBoundaryModel implementation).
    void setBoundaryModel(IBoundaryModel *model);

    /// @brief Sets the drag controller for boundary dragging.
    void setDragController(BoundaryDragController *ctrl) { m_dragController = ctrl; }

    /// @brief Sets the undo stack for boundary edit commands.
    void setUndoStack(QUndoStack *stack) { m_undoStack = stack; }

    /// @brief Sets the play widget for audio playback integration.
    void setPlayWidget(dstools::widgets::PlayWidget *pw) { m_playWidget = pw; }

    /// @brief Updates the viewport state.
    /// @param state New viewport state.
    void setViewport(const ViewportState &state);

    /// @brief Sets the playback cursor position.
    /// @param sec Playback position in seconds, or -1 if not playing.
    void setPlayhead(double sec);

    /// @brief Enables or disables the boundary overlay.
    /// @param enabled True to draw boundaries.
    void setBoundaryOverlayEnabled(bool enabled);

    /// @brief Triggers a repaint of the boundary overlay.
    void updateBoundaryOverlay();

signals:
    void positionClicked(double sec);            ///< Clicked position in seconds.
    void boundaryDragStarted(int tierIndex, int boundaryIndex);  ///< Boundary drag began.
    void boundaryDragging(int tierIndex, int boundaryIndex, TimePos newTime); ///< Boundary being dragged.
    void boundaryDragFinished(int tierIndex, int boundaryIndex, TimePos newTime); ///< Boundary drag ended.
    void hoveredBoundaryChanged(int boundaryIndex); ///< Hovered boundary changed.
    void entryScrollRequested(int delta);           ///< Scroll request from wheel event.

public slots:
    /// @brief Updates the playhead position from playback.
    /// @param sec Current playback position in seconds.
    void onPlayheadChanged(double sec);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void drawWaveform(QPainter &painter);               ///< Draws the waveform.
    void drawDbAxis(QPainter &painter);                  ///< Draws dB Y-axis labels.
    void drawPlayCursor(QPainter &painter);              ///< Draws the playback cursor.
    void drawBoundaryOverlay(QPainter &painter);         ///< Draws boundary lines.
    void rebuildMinMaxCache();                           ///< Rebuilds the min/max sample cache.

    [[nodiscard]] int hitTestBoundary(int x) const;     ///< Returns boundary index at x, or -1.

    /// @brief Finds the boundaries surrounding a time position.
    /// @param timeSec Time position in seconds.
    /// @param[out] outStart Start boundary time.
    /// @param[out] outEnd End boundary time.
    void findSurroundingBoundaries(double timeSec, double &outStart, double &outEnd) const;

    [[nodiscard]] double xToTime(int x) const;          ///< Converts pixel x to time.
    [[nodiscard]] int timeToX(double time) const;       ///< Converts time to pixel x.

    ViewportController *m_viewport = nullptr;           ///< Viewport controller.
    IBoundaryModel *m_boundaryModel = nullptr;          ///< Boundary model.
    BoundaryDragController *m_dragController = nullptr; ///< Drag controller.
    QUndoStack *m_undoStack = nullptr;                  ///< Undo stack.
    dstools::widgets::PlayWidget *m_playWidget = nullptr; ///< Play widget for audio playback.

    std::vector<float> m_samples;                       ///< Raw audio samples.
    int m_sampleRate = 44100;                           ///< Audio sample rate in Hz.

    /// @brief Cached min/max sample pair for fast waveform rendering.
    struct MinMaxPair {
        float min; ///< Minimum sample value in the block.
        float max; ///< Maximum sample value in the block.
    };
    std::vector<MinMaxPair> m_minMaxCache;              ///< Min/max cache array.
    double m_cacheResolution = 0;                       ///< Resolution of cached data.

    double m_viewStart = 0.0;                           ///< Visible range start in seconds.
    double m_viewEnd = 10.0;                            ///< Visible range end in seconds.

    double m_playhead = -1.0;                           ///< Playhead position, -1 if not playing.
    double m_amplitudeScale = 1.0;                       ///< Vertical amplitude zoom factor.
    bool m_boundaryOverlayEnabled = true;               ///< Whether boundary overlay is drawn.

    bool m_dragging = false;                            ///< Whether viewport is being scrolled.
    QPoint m_dragStartPos;                              ///< Mouse position at drag start.
    double m_dragStartTime = 0.0;                       ///< View start time at drag start.

    static constexpr int kBoundaryHitWidth = 8;         ///< Hit-test width in pixels.
    int m_hoveredBoundary = -1;                         ///< Hovered boundary index, or -1.
};

} // namespace phonemelabeler
} // namespace dstools
