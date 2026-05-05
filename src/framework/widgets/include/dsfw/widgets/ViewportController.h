#pragma once
/// @file ViewportController.h
/// @brief Resolution-driven time-axis viewport controller for waveform/spectrogram views.

#include <dsfw/widgets/WidgetsGlobal.h>
#include <QObject>

#include <vector>

namespace dsfw::widgets {

/// @brief Describes the visible time range and zoom level of a viewport.
struct DSFW_WIDGETS_API ViewportState {
    double startSec = 0.0;        ///< @brief Start of the visible range in seconds.
    double endSec = 10.0;         ///< @brief End of the visible range in seconds.
    double pixelsPerSecond = 200.0; ///< @brief Horizontal zoom level (derived from resolution).
};

/// @brief Controls the visible time range and zoom for time-axis views.
///
/// Internally driven by "resolution" (samples per pixel), following vLabeler's model.
/// The resolution is discrete and steps through a logarithmic table, eliminating
/// floating-point precision issues and ensuring hard zoom limits.
class DSFW_WIDGETS_API ViewportController : public QObject {
    Q_OBJECT
public:
    /// @brief Construct a ViewportController.
    /// @param parent Parent QObject.
    explicit ViewportController(QObject *parent = nullptr);

    // === Audio parameters (call once when audio is loaded) ===

    /// @brief Set audio parameters. Replaces setTotalDuration().
    /// @param sampleRate Audio sample rate in Hz.
    /// @param totalSamples Total number of samples.
    void setAudioParams(int sampleRate, int64_t totalSamples);

    /// @brief Set the total audio duration for clamping (legacy compatibility).
    /// @param duration Total duration in seconds.
    void setTotalDuration(double duration);

    /// @brief Get the total audio duration.
    /// @return Total duration in seconds.
    [[nodiscard]] double totalDuration() const;

    /// @brief Get the sample rate.
    [[nodiscard]] int sampleRate() const { return m_sampleRate; }

    /// @brief Get total samples.
    [[nodiscard]] int64_t totalSamples() const { return m_totalSamples; }

    // === Resolution (core zoom state) ===

    /// @brief Set the resolution (samples per pixel).
    /// @param resolution Samples per pixel (clamped to valid range).
    void setResolution(int resolution);

    /// @brief Get current resolution.
    [[nodiscard]] int resolution() const { return m_resolution; }

    /// @brief Zoom in (decrease resolution) centered on a time.
    /// @param centerSec Center time for zooming.
    void zoomIn(double centerSec);

    /// @brief Zoom out (increase resolution) centered on a time.
    /// @param centerSec Center time for zooming.
    void zoomOut(double centerSec);

    /// @brief Check if further zoom in is possible.
    [[nodiscard]] bool canZoomIn() const;

    /// @brief Check if further zoom out is possible.
    [[nodiscard]] bool canZoomOut() const;

    // === Legacy zoom API (maps to resolution changes) ===

    /// @brief Set the horizontal zoom level.
    /// @param pps Pixels per second.
    void setPixelsPerSecond(double pps);

    /// @brief Zoom in or out centered on a time position.
    /// @param centerSec Center time for zooming.
    /// @param factor Zoom factor (>1 zooms in, <1 zooms out).
    void zoomAt(double centerSec, double factor);

    // === Viewport (visible range) ===

    /// @brief Set the visible time range.
    /// @param startSec Start time in seconds.
    /// @param endSec End time in seconds.
    void setViewRange(double startSec, double endSec);

    /// @brief Scroll the viewport by a time delta.
    /// @param deltaSec Scroll amount in seconds.
    void scrollBy(double deltaSec);

    /// @brief Get the current viewport state.
    [[nodiscard]] const ViewportState &state() const { return m_state; }

    /// @brief Get the current zoom level (derived).
    [[nodiscard]] double pixelsPerSecond() const { return m_state.pixelsPerSecond; }

    /// @brief Get the start of the visible range.
    [[nodiscard]] double startSec() const { return m_state.startSec; }

    /// @brief Get the end of the visible range.
    [[nodiscard]] double endSec() const { return m_state.endSec; }

    /// @brief Get the center of the visible range.
    [[nodiscard]] double viewCenter() const { return (m_state.startSec + m_state.endSec) / 2.0; }

    /// @brief Get the visible duration.
    [[nodiscard]] double duration() const { return m_state.endSec - m_state.startSec; }

    /// @brief Get virtual canvas length in pixels.
    [[nodiscard]] double canvasLength() const { return static_cast<double>(m_totalSamples) / m_resolution; }

signals:
    /// @brief Emitted when the viewport range or zoom changes.
    /// @param state Updated viewport state.
    void viewportChanged(const dsfw::widgets::ViewportState &state);

private:
    ViewportState m_state;
    int m_sampleRate = 44100;
    int64_t m_totalSamples = 0;
    int m_resolution = 40;          ///< Samples per pixel (default matches vLabeler).

    /// Logarithmic resolution step table.
    static const std::vector<int> &resolutionTable();

    /// Find the index of the closest resolution in the table.
    int findResolutionIndex(int res) const;

    /// Current index in resolution table.
    int m_resolutionIndex = -1;

    void updatePPS();
    void clampAndEmit();
};

} // namespace dsfw::widgets
