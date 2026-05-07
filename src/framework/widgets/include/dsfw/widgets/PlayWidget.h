#pragma once
/// @file PlayWidget.h
/// @brief Audio playback control widget with transport buttons, slider, and device selection.

#include <dsfw/widgets/WidgetsGlobal.h>
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
class IAudioPlayer;
class AudioPlayer;
}

namespace dsfw::widgets {

/// @brief Audio playback control widget with transport buttons, position slider, and device menu.
class DSFW_WIDGETS_API PlayWidget : public QWidget {
    Q_OBJECT
public:
    /// @brief Construct a PlayWidget with an internal AudioPlayer.
    /// @param parent Parent widget.
    explicit PlayWidget(QWidget *parent = nullptr);

    /// @brief Construct a PlayWidget with an external audio player.
    /// @param player External audio player (not owned).
    /// @param parent Parent widget.
    explicit PlayWidget(dstools::audio::IAudioPlayer *player, QWidget *parent = nullptr);
    ~PlayWidget() override;

    /// @brief Open an audio file for playback.
    /// @param path Path to the audio file.
    void openFile(const QString &path);

    /// @brief Close the current audio file.
    void closeFile();

    /// @brief Check whether audio is currently playing.
    /// @return True if playing.
    bool isPlaying() const;

    /// @brief Start or stop playback.
    /// @param playing True to start, false to stop.
    void setPlaying(bool playing);

    /// @brief Seek to a time position.
    /// @param sec Position in seconds.
    void seek(double sec);

    /// @brief Get the total duration of the loaded audio.
    /// @return Duration in seconds.
    double duration() const;

    /// @brief Set a restricted playback range.
    /// @param startSec Range start in seconds.
    /// @param endSec Range end in seconds.
    void setPlayRange(double startSec, double endSec);

    /// @brief Clear the restricted playback range.
    void clearPlayRange();

signals:
    /// @brief Emitted periodically with the current playback position.
    /// @param positionSec Current position in seconds.
    void playheadChanged(double positionSec);

    /// @brief Emitted when this widget starts playing.
    /// Connect to AudioPlaybackManager::requestPlay for arbitration.
    void playRequested();

    /// @brief Emitted when this widget stops playing (user stop or playback end).
    /// Connect to AudioPlaybackManager::releasePlay for arbitration.
    void playStopped();

protected:
    void timerEvent(QTimerEvent *event) override;

private:
    QPushButton *m_playBtn;
    QPushButton *m_stopBtn;
    QPushButton *m_devBtn;
    QSlider *m_slider;
    QLabel *m_fileLabel;
    QLabel *m_timeLabel;
    QMenu *m_deviceMenu;
    QActionGroup *m_deviceActionGroup;

    dstools::audio::IAudioPlayer *m_player = nullptr;
    std::unique_ptr<dstools::audio::AudioPlayer> m_ownedPlayer;
    bool m_valid = false;

    int m_notifyTimerId = 0;
    QString m_filename;

    double m_rangeStart = 0.0;
    double m_rangeEnd = 0.0;
    bool m_hasRange = false;

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

} // namespace dsfw::widgets
