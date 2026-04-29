#pragma once

#include <dstools/WidgetsGlobal.h>
#include <QObject>

namespace dstools::widgets {

/// Snapshot of a viewport's visible range and zoom level.
struct DSTOOLS_WIDGETS_API ViewportState {
    double startSec = 0.0;       ///< Left edge of the visible range (seconds).
    double endSec = 10.0;        ///< Right edge of the visible range (seconds).
    double pixelsPerSecond = 200.0; ///< Horizontal zoom factor.
};

/// Manages a shared horizontal viewport for synchronized zoom/scroll across
/// multiple views. Mutators clamp the range to [0, totalDuration] and emit
/// viewportChanged() so that all connected views stay in sync.
class DSTOOLS_WIDGETS_API ViewportController : public QObject {
    Q_OBJECT
public:
    explicit ViewportController(QObject *parent = nullptr);

    /// Set the visible range by absolute start/end times (seconds).
    void setViewRange(double startSec, double endSec);
    /// Set the zoom level in pixels-per-second; view range is adjusted to keep
    /// the current center.
    void setPixelsPerSecond(double pps);
    /// Zoom by @p factor around a fixed anchor point @p centerSec.
    void zoomAt(double centerSec, double factor);
    /// Scroll by @p deltaSec seconds (positive = rightward).
    void scrollBy(double deltaSec);

    /// Set the total audio duration; used for clamping the visible range.
    void setTotalDuration(double duration) { m_totalDuration = duration; }
    /// Total audio duration (seconds).
    [[nodiscard]] double totalDuration() const { return m_totalDuration; }

    /// Current viewport state snapshot.
    [[nodiscard]] const ViewportState &state() const { return m_state; }
    /// Current zoom level (pixels per second).
    [[nodiscard]] double pixelsPerSecond() const { return m_state.pixelsPerSecond; }
    /// Left edge of the visible range (seconds).
    [[nodiscard]] double startSec() const { return m_state.startSec; }
    /// Right edge of the visible range (seconds).
    [[nodiscard]] double endSec() const { return m_state.endSec; }
    /// Center of the visible range (seconds).
    [[nodiscard]] double viewCenter() const { return (m_state.startSec + m_state.endSec) / 2.0; }
    /// Visible duration (seconds).
    [[nodiscard]] double duration() const { return m_state.endSec - m_state.startSec; }

signals:
    /// Emitted after any viewport mutation (zoom, scroll, range change).
    void viewportChanged(const dstools::widgets::ViewportState &state);

private:
    ViewportState m_state;
    double m_totalDuration = 0.0;
    double m_minPixelsPerSecond = 10.0;
    double m_maxPixelsPerSecond = 5000.0;
    void clampAndEmit();
};

} // namespace dstools::widgets
