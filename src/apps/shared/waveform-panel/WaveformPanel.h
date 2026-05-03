/// @file WaveformPanel.h
/// @brief Shared waveform display panel: time ruler + waveform + scrollbar + playback.
///
/// This is a standalone waveform viewer component used by both DsSlicerPage
/// and (eventually) PhonemeEditor. It renders audio waveform with boundary lines,
/// provides zoom/scroll/playback, but does NOT depend on TextGridDocument.

#pragma once

#include <QScrollBar>
#include <QWidget>
#include <vector>

#include <dstools/ViewportController.h>

namespace dstools {
namespace waveform {

class PlaybackController;

using dstools::widgets::ViewportController;
using dstools::widgets::ViewportState;

/// @brief Boundary data for overlay rendering and right-click playback.
struct BoundaryInfo {
    double timeSec = 0.0; ///< Boundary position in seconds.
};

/// @brief Shared waveform panel: TimeRuler + Waveform + Scrollbar + Playback.
///
/// Displays audio waveform with optional boundary overlay. Does NOT own or
/// know about TextGrid/phoneme documents. Boundary positions are set externally.
class WaveformPanel : public QWidget {
    Q_OBJECT

public:
    explicit WaveformPanel(QWidget *parent = nullptr);
    ~WaveformPanel() override;

    /// Load audio from file path.
    void loadAudio(const QString &path);

    /// Set audio sample data directly.
    void setAudioData(const std::vector<float> &samples, int sampleRate);

    /// Set boundary positions for overlay and right-click playback.
    void setBoundaries(const std::vector<BoundaryInfo> &boundaries);

    /// Access the viewport controller (for external synchronization).
    [[nodiscard]] ViewportController *viewport() const { return m_viewport; }

    /// Access the playback controller.
    [[nodiscard]] PlaybackController *playback() const { return m_playback; }

    /// Get current mono samples (for downstream processing).
    [[nodiscard]] const std::vector<float> &monoSamples() const { return m_samples; }

    /// Get sample rate.
    [[nodiscard]] int sampleRate() const { return m_sampleRate; }

    /// Get total duration in seconds.
    [[nodiscard]] double totalDuration() const;

signals:
    /// Emitted when audio is loaded successfully.
    void audioLoaded();

    /// Emitted when user clicks a position in the waveform.
    void positionClicked(double sec);

    /// Emitted when user right-clicks to play a segment.
    void segmentPlayRequested(double startSec, double endSec);

    /// Emitted when a boundary is clicked (for selection).
    void boundaryClicked(int index);

    /// Emitted on Ctrl+wheel zoom.
    void zoomChanged(double pixelsPerSecond);

private:
    class WaveformDisplay;
    class TimeRuler;

    ViewportController *m_viewport = nullptr;
    PlaybackController *m_playback = nullptr;
    TimeRuler *m_timeRuler = nullptr;
    WaveformDisplay *m_waveformDisplay = nullptr;
    QScrollBar *m_hScrollBar = nullptr;

    std::vector<float> m_samples;
    int m_sampleRate = 44100;
    std::vector<BoundaryInfo> m_boundaries;

    void buildLayout();
    void connectSignals();
    void updateScrollBar();
    void onScrollBarValueChanged(int value);
};

} // namespace waveform
} // namespace dstools
