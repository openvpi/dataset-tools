/// @file SpectrogramWidget.h
/// @brief Audio spectrogram visualization widget.

#pragma once

#include <QWidget>
#include <QImage>
#include <vector>
#include <QPoint>

#include <dstools/ViewportController.h>
#include <dstools/TimePos.h>
#include <dstools/PlayWidget.h>
#include "IBoundaryModel.h"
#include "BoundaryBindingManager.h"
#include "SpectrogramColorPalette.h"

class QPainter;
class QUndoStack;

namespace dstools {
namespace phonemelabeler {

using dstools::widgets::ViewportController;
using dstools::widgets::ViewportState;


/// @brief Renders FFT-based spectrogram with configurable color palettes,
///        synchronized viewport, and boundary dragging.
class SpectrogramWidget : public QWidget {
    Q_OBJECT

public:
    /// @brief Constructs the spectrogram widget.
    /// @param viewport Viewport controller for time synchronization.
    /// @param parent Optional parent widget.
    explicit SpectrogramWidget(ViewportController *viewport, QWidget *parent = nullptr);
    ~SpectrogramWidget() override;

    /// @brief Sets the audio sample data for spectrogram computation.
    /// @param samples Audio samples.
    /// @param sampleRate Sample rate in Hz.
    void setAudioData(const std::vector<float> &samples, int sampleRate);

    /// @brief Updates the viewport state.
    /// @param state New viewport state.
    void setViewport(const ViewportState &state);

    /// @brief Sets the boundary model for boundary display.
    void setBoundaryModel(IBoundaryModel *model) { m_boundaryModel = model; }

    /// @brief Sets the boundary binding manager.
    void setBindingManager(BoundaryBindingManager *mgr) { m_bindingMgr = mgr; }

    /// @brief Sets the undo stack for boundary edit commands.
    void setUndoStack(QUndoStack *stack) { m_undoStack = stack; }

    /// @brief Sets the play widget for right-click segment playback.
    void setPlayWidget(dstools::widgets::PlayWidget *pw) { m_playWidget = pw; }

    /// @brief Sets the color palette for spectrogram rendering.
    /// @param palette Color palette to use.
    void setColorPalette(const SpectrogramColorPalette &palette);

    /// @brief Returns the current color palette.
    [[nodiscard]] const SpectrogramColorPalette &colorPalette() const { return m_palette; }

    /// @brief Triggers a repaint of the boundary overlay.
    void updateBoundaryOverlay();

signals:
    void boundaryDragStarted(int tierIndex, int boundaryIndex);  ///< Boundary drag began.
    void boundaryDragging(int tierIndex, int boundaryIndex, TimePos newTime); ///< Boundary being dragged.
    void boundaryDragFinished(int tierIndex, int boundaryIndex, TimePos newTime); ///< Boundary drag ended.
    void hoveredBoundaryChanged(int boundaryIndex); ///< Hovered boundary changed.
    void entryScrollRequested(int delta);           ///< Scroll request from wheel event.
    void visibleStateChanged(bool visible);         ///< Emitted when widget visibility changes externally.

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void prepareSpectrogramParams();                    ///< Prepares FFT parameters without computing.
    void computeSpectrogramRange(int frameStart, int frameEnd); ///< Computes FFT for a range of frames.
    void ensureSpectrogramRange(int frameStart, int frameEnd);  ///< Ensures frames are computed (lazy).
    void rebuildViewImage();                            ///< Rebuilds the cached view image.
    static std::vector<double> makeBlackmanHarrisWindow(int N); ///< Creates a Blackman-Harris window.
    [[nodiscard]] QRgb intensityToColor(float normalized) const; ///< Maps normalized intensity to color.

    void drawBoundaryOverlay(QPainter &painter);        ///< Draws boundary lines.

    [[nodiscard]] int hitTestBoundary(int x) const;     ///< Returns boundary index at x, or -1.
    void startBoundaryDrag(int boundaryIndex, TimePos time);   ///< Begins boundary drag.
    void updateBoundaryDrag(TimePos currentTime);              ///< Updates drag position.
    void endBoundaryDrag(TimePos finalTime);                   ///< Commits boundary drag.

    /// @brief Finds the boundaries surrounding a time position.
    void findSurroundingBoundaries(double timeSec, double &outStart, double &outEnd) const;

    [[nodiscard]] double xToTime(int x) const;          ///< Converts pixel x to time.
    [[nodiscard]] int timeToX(double time) const;       ///< Converts time to pixel x.

    ViewportController *m_viewport = nullptr;           ///< Viewport controller.
    IBoundaryModel *m_boundaryModel = nullptr;          ///< Boundary model.
    BoundaryBindingManager *m_bindingMgr = nullptr;     ///< Binding manager.
    QUndoStack *m_undoStack = nullptr;                  ///< Undo stack.
    dstools::widgets::PlayWidget *m_playWidget = nullptr; ///< Play widget for right-click playback.

    std::vector<float> m_samples;                       ///< Raw audio samples.
    int m_sampleRate = 44100;                           ///< Audio sample rate in Hz.
    SpectrogramColorPalette m_palette = SpectrogramColorPalette::orangeYellow(); ///< Active color palette.

    std::vector<std::vector<float>> m_spectrogramData;  ///< Spectrogram data [frame][bin], normalized 0..1.
    std::vector<bool> m_computedFrames;                 ///< Tracks which frames have been computed.
    std::vector<double> m_fftWindow;                    ///< Precomputed FFT window.
    int m_totalFrames = 0;                              ///< Total number of frames in the audio.
    int m_hopSize = 110;                                ///< Hop size in samples.
    int m_windowSize = 512;                             ///< FFT window size in samples.
    int m_numFreqBins = 0;                              ///< Number of frequency bins.

    QImage m_viewImage;                                 ///< Cached rendered view image.
    double m_cachedViewStart = -1.0;                    ///< Cached view start time.
    double m_cachedViewEnd = -1.0;                      ///< Cached view end time.
    int m_cachedWidth = 0;                              ///< Cached image width.
    int m_cachedHeight = 0;                             ///< Cached image height.

    double m_viewStart = 0.0;                           ///< Visible range start in seconds.
    double m_viewEnd = 10.0;                            ///< Visible range end in seconds.

    static constexpr int kStandardHopSize = 256;        ///< Standard hop size.
    static constexpr int kStandardWindowSize = 2048;    ///< Standard FFT window size.
    static constexpr double kStandardSampleRate = 44100.0; ///< Standard sample rate.
    static constexpr double kMaxFrequency = 8000.0;     ///< Maximum displayed frequency in Hz.
    static constexpr double kMinIntensityDb = -80.0;    ///< Minimum intensity in dB.
    static constexpr double kMaxIntensityDb = 0.0;      ///< Maximum intensity in dB.

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
