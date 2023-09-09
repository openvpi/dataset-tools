#ifndef PLAYWIDGET_H
#define PLAYWIDGET_H

#include <QAction>
#include <QBoxLayout>
#include <QFileSystemModel>
#include <QLabel>
#include <QMenu>
#include <QPluginLoader>
#include <QPushButton>
#include <QSet>
#include <QSlider>
#include <QTreeView>

#include "Api/IAudioDecoder.h"
#include "Api/IAudioPlayback.h"

#include "Api/interfaces/IAudioDecoderPlugin.h"
#include "Api/interfaces/IAudioPlaybackPlugin.h"

class PlayWidget : public QWidget {
    Q_OBJECT
public:
    PlayWidget(QWidget *parent = nullptr);
    ~PlayWidget() override;

    void openFile(const QString &filename);

    bool isPlaying() const;
    void setPlaying(bool playing);

protected:
    // QPluginLoader decoderLoader, playbackLoader;

    // QsApi::IAudioDecoderPlugin *decoder_plugin;
    // QsApi::IAudioPlaybackPlugin *playback_plugin;
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

    void timerEvent(QTimerEvent *event) override;

private:
    void initPlugins();
    void uninitPlugins();

    void reloadDevices();
    void reloadButtonStatus();
    void reloadSliderStatus();
    void reloadDeviceActionStatus();

    void _q_playButtonClicked();
    void _q_stopButtonClicked();
    void _q_devButtonClicked();
    void _q_sliderReleased();
    void _q_deviceActionTriggered(QAction *action);
    void _q_playStateChanged();
    void _q_audioDeviceChanged();
    void _q_audioDeviceAdded();
    void _q_audioDeviceRemoved();
};

#endif // PLAYWIDGET_H
