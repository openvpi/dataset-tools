#pragma once
/// @file ViewportController.h
/// @brief Time-axis viewport controller for waveform/spectrogram views.

#include <dsfw/widgets/WidgetsGlobal.h>
#include <QObject>

namespace dsfw::widgets {

/// @brief Describes the visible time range and zoom level of a viewport.
struct DSFW_WIDGETS_API ViewportState {
    double startSec = 0.0;        ///< @brief Start of the visible range in seconds.
    double endSec = 10.0;         ///< @brief End of the visible range in seconds.
    double pixelsPerSecond = 200.0; ///< @brief Horizontal zoom level.
};

/// @brief Controls the visible time range and zoom for time-axis views.
class DSFW_WIDGETS_API ViewportController : public QObject {
    Q_OBJECT
public:
    /// @brief Construct a ViewportController.
    /// @param parent Parent QObject.
    explicit ViewportController(QObject *parent = nullptr);

    /// @brief Set the visible time range.
    /// @param startSec Start time in seconds.
    /// @param endSec End time in seconds.
    void setViewRange(double startSec, double endSec);

    /// @brief Set the horizontal zoom level.
    /// @param pps Pixels per second.
    void setPixelsPerSecond(double pps);

    /// @brief Zoom in or out centered on a time position.
    /// @param centerSec Center time for zooming.
    /// @param factor Zoom factor (>1 zooms in, <1 zooms out).
    void zoomAt(double centerSec, double factor);

    /// @brief Scroll the viewport by a time delta.
    /// @param deltaSec Scroll amount in seconds.
    void scrollBy(double deltaSec);

    /// @brief Set the total audio duration for clamping.
    /// @param duration Total duration in seconds.
    void setTotalDuration(double duration) { m_totalDuration = duration; }

    /// @brief Get the total audio duration.
    /// @return Total duration in seconds.
    [[nodiscard]] double totalDuration() const { return m_totalDuration; }

    /// @brief Get the current viewport state.
    /// @return Reference to the viewport state.
    [[nodiscard]] const ViewportState &state() const { return m_state; }

    /// @brief Get the current zoom level.
    /// @return Pixels per second.
    [[nodiscard]] double pixelsPerSecond() const { return m_state.pixelsPerSecond; }

    /// @brief Get the start of the visible range.
    /// @return Start time in seconds.
    [[nodiscard]] double startSec() const { return m_state.startSec; }

    /// @brief Get the end of the visible range.
    /// @return End time in seconds.
    [[nodiscard]] double endSec() const { return m_state.endSec; }

    /// @brief Get the center of the visible range.
    /// @return Center time in seconds.
    [[nodiscard]] double viewCenter() const { return (m_state.startSec + m_state.endSec) / 2.0; }

    /// @brief Get the visible duration.
    /// @return Duration in seconds.
    [[nodiscard]] double duration() const { return m_state.endSec - m_state.startSec; }

signals:
    /// @brief Emitted when the viewport range or zoom changes.
    /// @param state Updated viewport state.
    void viewportChanged(const dsfw::widgets::ViewportState &state);

private:
    ViewportState m_state;
    double m_totalDuration = 0.0;
    double m_minPixelsPerSecond = 10.0;
    double m_maxPixelsPerSecond = 5000.0;
    void clampAndEmit();
};

} // namespace dsfw::widgets
