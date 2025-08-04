#ifndef PLAYWIDGET_H
#define PLAYWIDGET_H

#include <QAction>
#include <QBoxLayout>
#include <QFileSystemModel>
#include <QLabel>
#include <QMenu>
#include <QPluginLoader>
#include <QPushButton>
#include <QTreeView>
#include <chrono>

#include "Api/IAudioDecoder.h"
#include "Api/IAudioPlayback.h"

namespace SlurCutter {
    class PlayWidget final : public QWidget {
        Q_OBJECT
    public:
        explicit PlayWidget(QWidget *parent = nullptr);
        ~PlayWidget() override;

        void openFile(const QString &filename);

        bool isPlaying() const;
        void setPlaying(bool playing);

        void setRange(double start, double end);

    signals:
        void playheadChanged(double position);

    protected:
        QsApi::IAudioDecoder *decoder;
        QsApi::IAudioPlayback *playback;

        QMenu *deviceMenu;
        QActionGroup *deviceActionGroup;

        int notifyTimerId;
        bool playing;
        QString filename;

        QLabel *fileLabel;
        QSlider *slider;
        QLabel *timeLabel;

        QPushButton *playButton;
        QPushButton *stopButton;
        QPushButton *devButton;

        QVBoxLayout *mainLayout;
        QHBoxLayout *buttonsLayout;


        // Manually maintained time
        uint64_t lastObtainedTimeMs = 0, pauseAtTime = 0;
        std::chrono::time_point<std::chrono::steady_clock> lastObtainedTimePoint;
        uint64_t estimatedTimeMs() const;

        // Limited time range
        double rangeBegin = 0.0, rangeEnd = 0.0;

        void timerEvent(QTimerEvent *event) override;

    private:
        void initPlugins();
        void uninitPlugins() const;

        void reloadDevices() const;
        void reloadButtonStatus() const;
        void reloadSliderStatus();
        void reloadDeviceActionStatus() const;
        void reloadFinePlayheadStatus(uint64_t timeMs = UINT64_MAX);

        void _q_playButtonClicked();
        void _q_stopButtonClicked();
        void _q_devButtonClicked() const;
        void _q_sliderReleased();
        void _q_deviceActionTriggered(const QAction *action);
        void _q_playStateChanged();
        void _q_audioDeviceChanged() const;
        void _q_audioDeviceAdded() const;
        void _q_audioDeviceRemoved() const;
    };
}
#endif // PLAYWIDGET_H
