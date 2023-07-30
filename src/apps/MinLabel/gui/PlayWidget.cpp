#include "PlayWidget.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QPluginLoader>
#include <QTime>

// https://iconduck.com/icons

class MySlider : public QSlider {
public:
    explicit MySlider(Qt::Orientation orientation, QWidget *parent = nullptr) : QSlider(orientation, parent) {
    }

protected:
    void mousePressEvent(QMouseEvent *ev) override {
        bool isMoved = false;

        auto conn = connect(this, &QSlider::actionTriggered, this, [&](int action) {
            switch (action) {
                case QSlider::SliderMove:
                case QSlider::SliderPageStepAdd:
                case QSlider::SliderPageStepSub:
                    isMoved = true;
                    break;
                default:
                    break;
            }
        });

        QSlider::mousePressEvent(ev);

        disconnect(conn);

        if (isMoved) {
            QStyleOptionSlider opt;
            initStyleOption(&opt);
            const QRect sliderRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
            const QPoint center = sliderRect.center() - sliderRect.topLeft();
            setSliderPosition(pixelPosToRangeValue(pick(ev->pos() - center)));
            emit sliderReleased();
        }
    }

private:
    int pixelPosToRangeValue(int pos) const {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        QRect gr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
        QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
        int sliderMin, sliderMax, sliderLength;

        if (orientation() == Qt::Horizontal) {
            sliderLength = sr.width();
            sliderMin = gr.x();
            sliderMax = gr.right() - sliderLength + 1;
        } else {
            sliderLength = sr.height();
            sliderMin = gr.y();
            sliderMax = gr.bottom() - sliderLength + 1;
        }
        return QStyle::sliderValueFromPosition(minimum(), maximum(), pos - sliderMin, sliderMax - sliderMin,
                                               opt.upsideDown);
    }

    inline int pick(const QPoint &pt) const {
        return orientation() == Qt::Horizontal ? pt.x() : pt.y();
    }
};

PlayWidget::PlayWidget(QWidget *parent) : QWidget(parent) {
    notifyTimerId = 0;
    playing = false;

    initPlugins();

    // Init menu
    deviceMenu = new QMenu(this);
    deviceActionGroup = new QActionGroup(this);
    deviceActionGroup->setExclusive(true);

    // Init central
    fileLabel = new QLabel("Select audio file.");
    timeLabel = new QLabel("--:--/--:--");

    slider = new MySlider(Qt::Horizontal);
    slider->setRange(0, 10000);

    playButton = new QPushButton();
    playButton->setProperty("type", "user");
    playButton->setObjectName("play-button");

    stopButton = new QPushButton();
    stopButton->setProperty("type", "user");
    stopButton->setObjectName("stop-button");
    stopButton->setIcon(QIcon(":/res/stop.svg"));

    devButton = new QPushButton();
    devButton->setProperty("type", "user");
    devButton->setObjectName("dev-button");
    devButton->setIcon(QIcon(":/res/audio.svg"));

    buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(playButton);
    buttonsLayout->addWidget(stopButton);
    buttonsLayout->addWidget(devButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(timeLabel);

    mainLayout = new QVBoxLayout();
    mainLayout->addWidget(fileLabel);
    mainLayout->addWidget(slider);
    mainLayout->addLayout(buttonsLayout);

    setLayout(mainLayout);

    connect(playButton, &QPushButton::clicked, this, &PlayWidget::_q_playButtonClicked);
    connect(stopButton, &QPushButton::clicked, this, &PlayWidget::_q_stopButtonClicked);
    connect(devButton, &QPushButton::clicked, this, &PlayWidget::_q_devButtonClicked);
    connect(slider, &QSlider::sliderReleased, this, &PlayWidget::_q_sliderReleased);
    connect(deviceMenu, &QMenu::triggered, this, &PlayWidget::_q_deviceActionTriggered);

    connect(playback, &QsApi::IAudioPlayback::stateChanged, this, &PlayWidget::_q_playStateChanged);
    connect(playback, &QsApi::IAudioPlayback::deviceChanged, this, &PlayWidget::_q_audioDeviceChanged);
    connect(playback, &QsApi::IAudioPlayback::deviceAdded, this, &PlayWidget::_q_audioDeviceAdded);
    connect(playback, &QsApi::IAudioPlayback::deviceRemoved, this, &PlayWidget::_q_audioDeviceRemoved);

    reloadDevices();
    reloadButtonStatus();
}

PlayWidget::~PlayWidget() {
    playback->dispose();
    decoder->close();
    uninitPlugins();
}

void PlayWidget::openFile(const QString &filename) {
    setPlaying(false);
    if (decoder->isOpen()) {
        decoder->SetPosition(0);
        reloadSliderStatus();
        decoder->close();
        timeLabel->setText("--:--/--:--");
    }

    if (!decoder->open(filename, {44100, QsMedia::AV_SAMPLE_FMT_FLT, 2})) {
        QMessageBox::critical(this, qApp->applicationName(), "Failed to initialize decoder!");
        return;
    }

    fileLabel->setText(QDir::toNativeSeparators(filename));
    this->filename = filename;

    slider->setMinimum(0);
    slider->setMaximum(decoder->Length());

    reloadSliderStatus();
}

bool PlayWidget::isPlaying() const {
    return playing;
}

void PlayWidget::setPlaying(bool playing) {
    if (this->playing == playing) {
        return;
    }

    this->playing = playing;

    if (playing) {
        playback->play();

        notifyTimerId = this->startTimer(20);
    } else {
        playback->stop();

        killTimer(notifyTimerId);
        notifyTimerId = 0;
    }

    reloadButtonStatus();
    reloadSliderStatus();
}

void PlayWidget::timerEvent(QTimerEvent *event) {
    if (event->timerId() == notifyTimerId) {
        reloadSliderStatus();
    }
}

#include "FFmpegDecoder.h"
#include "SDLPlayback.h"

void PlayWidget::initPlugins() {
    // #ifdef Q_OS_LINUX
    //     decoderLoader.setFileName("audiodecoders/libFFmpegDecoder");
    //     playbackLoader.setFileName("audioplaybacks/libSDLPlayback");
    // #else
    //     decoderLoader.setFileName("audiodecoders/FFmpegDecoder");
    //     playbackLoader.setFileName("audioplaybacks/SDLPlayback");
    // #endif

    //     decoder = nullptr;
    //     playback = nullptr;

    // decoder_plugin = qobject_cast<QsApi::IAudioDecoderPlugin *>(decoderLoader.instance());
    // playback_plugin = qobject_cast<QsApi::IAudioPlaybackPlugin *>(playbackLoader.instance());

    // if (!decoder_plugin || !playback_plugin) {
    //     QMessageBox::critical(
    //         this, qApp->applicationName(),
    //         QString("Failed to load plugins: %1!").arg(decoderLoader.errorString()));
    //     goto out;
    // }

    // decoder = decoder_plugin->create({}, this);
    // if (!decoder) {
    //     QMessageBox::critical(this, qApp->applicationName(), QString("Failed to create decoder object!"));
    //     goto out2;
    // }

    // playback = playback_plugin->create({}, this);
    // if (!playback) {
    //     QMessageBox::critical(this, qApp->applicationName(), QString("Failed to create playback object!"));
    //     goto out2;
    // }

    decoder = new FFmpegDecoder();
    playback = new SDLPlayback();

    if (!playback->setup(QsMedia::PlaybackArguments{44100, 2, 1024})) {
        QMessageBox::critical(this, qApp->applicationName(),
                              QString("Failed to load playback: %1!")
                                  .arg(
                                      // playbackLoader.errorString()
                                      "SDLPlayback"));
        goto out;
    }
    playback->setDecoder(decoder);
    return;

out2:
    // Must delete before uninitializing the plugins.
    // If rely on the destructor of QWidget we would be calling unloaded code upon exit and crash the program
    delete playback;
    delete decoder;

out:
    uninitPlugins();
    ::exit(-1);
}

void PlayWidget::uninitPlugins() {
    delete playback;
    delete decoder;
    // decoderLoader.unload();
    // playbackLoader.unload();
}

void PlayWidget::reloadDevices() {
    deviceMenu->clear();

    QStringList devices = playback->devices();
    for (const QString &dev : qAsConst(devices)) {
        auto action = new QAction(dev, deviceMenu);
        action->setCheckable(true);
        action->setData(dev);
        deviceMenu->addAction(action);
        deviceActionGroup->addAction(action);
    }

    reloadDeviceActionStatus();
}

void PlayWidget::reloadButtonStatus() {
    playButton->setIcon(QIcon(!playing ? ":/res/play.svg" : ":/res/pause.svg"));
}

void PlayWidget::reloadSliderStatus() {
    qint64 max = decoder->Length();
    qint64 pos = decoder->Position();

    if (!slider->isSliderDown()) {
        slider->setValue(pos);
    }

    auto fmt = decoder->Format();
    int len_msecs = (double(max) / fmt.SampleRate() / 4 / fmt.Channels()) * 1000;
    int pos_msecs = (double(pos) / fmt.SampleRate() / 4 / fmt.Channels()) * 1000;

    QTime time(0, 0, 0);
    timeLabel->setText(time.addMSecs(pos_msecs).toString("mm:ss") + "/" + time.addMSecs(len_msecs).toString("mm:ss"));
}

void PlayWidget::reloadDeviceActionStatus() {
    QString dev = playback->currentDevice();
    const auto &actions = deviceMenu->actions();

    for (QAction *action : actions) {
        action->setChecked(false);
    }
    for (QAction *action : actions) {
        if (action->data().toString() == dev) {
            action->setChecked(true);
            break;
        }
    }
}

void PlayWidget::_q_playButtonClicked() {
    if (!decoder->isOpen()) {
        return;
    }
    setPlaying(!playing);
}

void PlayWidget::_q_stopButtonClicked() {
    if (!decoder->isOpen()) {
        return;
    }
    decoder->SetPosition(0);
    setPlaying(false);
}

void PlayWidget::_q_devButtonClicked() {
    deviceMenu->exec(QCursor::pos());
}

void PlayWidget::_q_sliderReleased() {
    if (!decoder->isOpen()) {
        slider->setValue(0);
        return;
    }

    double percentage = double(slider->value()) / slider->maximum();

    decoder->SetPosition(decoder->Length() * percentage);
    setPlaying(true);
}

void PlayWidget::_q_deviceActionTriggered(QAction *action) {
    if (playing) {
        QMessageBox::warning(this, qApp->applicationName(), "Stop sound first!");
    } else {
        QString dev = action->data().toString();
        playback->setDevice(dev);
    }

    reloadDeviceActionStatus();
}

void PlayWidget::_q_playStateChanged() {
    bool isPlaying = playback->isPlaying();
    if (playing != isPlaying) {
        if (decoder->Position() == decoder->Length()) {
            // Sound complete
            decoder->SetPosition(0);
        }
        setPlaying(isPlaying);
    }
}

void PlayWidget::_q_audioDeviceChanged() {
    reloadDeviceActionStatus();
}

void PlayWidget::_q_audioDeviceAdded() {
    reloadDevices();
}

void PlayWidget::_q_audioDeviceRemoved() {
    reloadDevices();
}
