#ifndef PLAYWIDGET_H
#define PLAYWIDGET_H

#include <QBoxLayout>
#include <QFileSystemModel>
#include <QLabel>
#include <QMenu>
#include <QPluginLoader>
#include <QPushButton>
#include <QTreeView>

#include "Api/IAudioDecoder.h"
#include "Api/IAudioPlayback.h"

class PlayWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlayWidget(QWidget *parent = nullptr);
    ~PlayWidget() override;

    void openFile(const QString &filename);

    bool isPlaying() const;
    void setPlaying(bool playing);

protected:
    QsApi::IAudioDecoder *decoder{};
    QsApi::IAudioPlayback *playback{};

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
    void uninitPlugins() const;

    void reloadDevices();
    void reloadButtonStatus() const;
    void reloadSliderStatus() const;
    void reloadDeviceActionStatus() const;

    void _q_playButtonClicked();
    void _q_stopButtonClicked();
    void _q_devButtonClicked() const;
    void _q_sliderReleased();
    void _q_deviceActionTriggered(const QAction *action);
    void _q_playStateChanged();
    void _q_audioDeviceChanged() const;
    void _q_audioDeviceAdded();
    void _q_audioDeviceRemoved();
};

#endif // PLAYWIDGET_H
