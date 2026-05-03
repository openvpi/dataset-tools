/// @file PlaybackController.h
/// @brief Audio playback controller wrapping PlayWidget for right-click play behavior.

#pragma once

#include <QObject>
#include <dstools/PlayWidget.h>

namespace dstools {
namespace waveform {

/// @brief Wraps PlayWidget to provide segment-based playback without visible UI.
///
/// Used by WaveformPanel for right-click-to-play behavior. The PlayWidget
/// instance is kept hidden; playback is triggered programmatically.
class PlaybackController : public QObject {
    Q_OBJECT

public:
    explicit PlaybackController(QObject *parent = nullptr);
    ~PlaybackController() override;

    /// Open an audio file for playback.
    void openFile(const QString &path);

    /// Play a segment defined by start and end times (seconds).
    void playSegment(double startSec, double endSec);

    /// Stop playback.
    void stop();

    /// @return true if currently playing.
    [[nodiscard]] bool isPlaying() const;

    /// Access the underlying PlayWidget (for signal connections).
    [[nodiscard]] dstools::widgets::PlayWidget *playWidget() const { return m_playWidget; }

signals:
    /// Emitted when the playhead position changes during playback.
    void playheadChanged(double sec);

private:
    dstools::widgets::PlayWidget *m_playWidget = nullptr;
};

} // namespace waveform
} // namespace dstools
