#pragma once
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

class DSFW_WIDGETS_API PlayWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlayWidget(QWidget *parent = nullptr);
    explicit PlayWidget(dstools::audio::IAudioPlayer *player, QWidget *parent = nullptr);
    ~PlayWidget() override;

    void openFile(const QString &path);
    void closeFile();
    bool isPlaying() const;
    void setPlaying(bool playing);

    void seek(double sec);

    double duration() const;

    void setPlayRange(double startSec, double endSec);
    void clearPlayRange();

signals:
    void playheadChanged(double positionSec);

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
