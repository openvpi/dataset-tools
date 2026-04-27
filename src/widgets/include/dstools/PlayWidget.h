#pragma once
#include <dstools/WidgetsGlobal.h>
#include <QWidget>
#include <chrono>
#include <memory>

class QPushButton;
class QSlider;
class QLabel;
class QMenu;
class QActionGroup;
class QAction;

namespace dstools::audio {
class AudioDecoder;
class AudioPlayback;
}

namespace dstools::widgets {

/// Unified audio playback widget. Merges original MinLabel::PlayWidget and SlurCutter::PlayWidget.
class DSTOOLS_WIDGETS_API PlayWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlayWidget(QWidget *parent = nullptr);
    ~PlayWidget() override;

    void openFile(const QString &path);
    void closeFile();
    bool isPlaying() const;
    void setPlaying(bool playing);

    /// Set play range (seconds). After setting, only audio in this range is played.
    void setPlayRange(double startSec, double endSec);
    void clearPlayRange();

signals:
    /// Playhead position changed (seconds). Only emitted when play range is set.
    void playheadChanged(double positionSec);

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    // UI components
    QPushButton *m_playBtn;
    QPushButton *m_stopBtn;
    QPushButton *m_devBtn;
    QSlider *m_slider;
    QLabel *m_fileLabel;
    QLabel *m_timeLabel;
    QMenu *m_deviceMenu;
    QActionGroup *m_deviceActionGroup;

    // Audio backend
    dstools::audio::AudioDecoder *m_decoder = nullptr;
    dstools::audio::AudioPlayback *m_playback = nullptr;
    bool m_valid = false;  // BUG-001 fix

    // State
    int m_notifyTimerId = 0;
    QString m_filename;

    // Range playback
    double m_rangeStart = 0.0;
    double m_rangeEnd = 0.0;
    bool m_hasRange = false;

    // Playhead tracking (SlurCutter steady_clock precision)
    uint64_t m_lastObtainedTimeMs = 0;
    std::chrono::time_point<std::chrono::steady_clock> m_lastObtainedTimePoint;
    uint64_t estimatedTimeMs() const;

    void initAudio();
    void uninitAudio();
    void reloadDevices();
    void reloadButtonStatus();
    void reloadSliderStatus();
    void reloadDeviceActionStatus();
    void reloadFinePlayheadStatus(uint64_t timeMs = UINT64_MAX);

    void onPlayClicked();
    void onStopClicked();
    void onDevClicked();
    void onSliderReleased();
    void onDeviceAction(QAction *action);
    void onPlaybackStateChanged();
    void onDeviceChanged();
};

} // namespace dstools::widgets
