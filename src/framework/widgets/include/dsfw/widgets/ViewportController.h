#pragma once
/// @file ViewportController.h
/// @brief Resolution-driven time-axis viewport controller for waveform/spectrogram views.
///
/// The sole zoom state is "resolution" (samples per pixel). All time↔pixel
/// conversions use resolution + sampleRate. The legacy "pixelsPerSecond"
/// concept is intentionally absent — see D-26 in human-decisions.md.

#include <QObject>
#include <dsfw/widgets/WidgetsGlobal.h>

namespace dsfw::widgets {

    /// @brief Describes the visible time range and zoom level of a viewport.
    ///
    /// Widgets receive this struct via viewportChanged(). To convert between
    /// time and pixel coordinates, use:
    ///   pixelX  = (timeSec - startSec) * sampleRate / resolution
    ///   timeSec = pixelX * resolution / sampleRate + startSec
    struct DSFW_WIDGETS_API ViewportState {
        double startSec = 0.0;  ///< Start of the visible range in seconds.
        double endSec = 10.0;   ///< End of the visible range in seconds.
        int resolution = 40;    ///< Samples per pixel (zoom level).
        int sampleRate = 44100; ///< Audio sample rate in Hz.
    };

    /// @brief Controls the visible time range and zoom for time-axis views.
    ///
    /// Zoom is driven by "resolution" (samples per pixel). Resolution is a
    /// continuous integer >= kMinResolution (10), rounded to the nearest tens.
    /// No discrete table — stepless zoom with round-tens stays predictable.
    class DSFW_WIDGETS_API ViewportController : public QObject {
        Q_OBJECT
    public:
        static constexpr int kMinResolution = 10;

        /// @brief Construct a ViewportController.
        /// @param parent Parent QObject.
        explicit ViewportController(QObject *parent = nullptr);

        // === Audio parameters (call once when audio is loaded) ===

        /// @brief Set audio parameters.
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
        [[nodiscard]] int sampleRate() const {
            return m_sampleRate;
        }

        /// @brief Get total samples.
        [[nodiscard]] int64_t totalSamples() const {
            return m_totalSamples;
        }

        // === Resolution (core zoom state) ===

        /// @brief Set the resolution (samples per pixel).
        /// Rounds to the nearest tens and clamps to >= kMinResolution.
        /// @param resolution Samples per pixel.
        void setResolution(int resolution);

        /// @brief Get current resolution (samples per pixel).
        [[nodiscard]] int resolution() const {
            return m_resolution;
        }

        /// @brief Zoom in (decrease resolution) centered on a time.
        /// @param centerSec Center time for zooming.
        void zoomIn(double centerSec);

        /// @brief Zoom out (increase resolution) centered on a time.
        /// @param centerSec Center time for zooming.
        void zoomOut(double centerSec);

        /// @brief Check if further zoom in is possible.
        [[nodiscard]] bool canZoomIn() const;

        /// @brief Check if further zoom out is possible.
        /// Always true — there is no hard upper bound (stepless zoom).
        [[nodiscard]] bool canZoomOut() const;

        /// @brief Zoom in or out centered on a time position (convenience wrapper).
        /// @param centerSec Center time for zooming.
        /// @param factor Zoom factor (>1 zooms in, <1 zooms out).
        void zoomAt(double centerSec, double factor);

        /// @brief Set resolution from a pixels-per-second value (legacy compatibility).
        /// Converts pps to resolution and rounds to nearest tens.
        /// @param pps Pixels per second.
        void setPixelsPerSecond(double pps);

        // === Viewport (visible range) ===

        /// @brief Set the visible time range.
        /// @param startSec Start time in seconds.
        /// @param endSec End time in seconds.
        void setViewRange(double startSec, double endSec);

        /// @brief Scroll the viewport by a time delta.
        /// @param deltaSec Scroll amount in seconds.
        void scrollBy(double deltaSec);

        /// @brief Get the current viewport state.
        [[nodiscard]] const ViewportState &state() const {
            return m_state;
        }

        /// @brief Get the start of the visible range.
        [[nodiscard]] double startSec() const {
            return m_state.startSec;
        }

        /// @brief Get the end of the visible range.
        [[nodiscard]] double endSec() const {
            return m_state.endSec;
        }

        /// @brief Get the center of the visible range.
        [[nodiscard]] double viewCenter() const {
            return (m_state.startSec + m_state.endSec) / 2.0;
        }

        /// @brief Get the visible duration.
        [[nodiscard]] double duration() const {
            return m_state.endSec - m_state.startSec;
        }

        /// @brief Get virtual canvas length in pixels.
        [[nodiscard]] double canvasLength() const {
            return static_cast<double>(m_totalSamples) / m_resolution;
        }

    signals:
        /// @brief Emitted when the viewport range or zoom changes.
        /// @param state Updated viewport state.
        void viewportChanged(const ViewportState &state);

    private:
        ViewportState m_state;
        int m_sampleRate = 44100;
        int64_t m_totalSamples = 0;
        int m_resolution = 40; ///< Samples per pixel.

        void syncStateFields();
        void clampAndEmit();
    };

} // namespace dsfw::widgets
