#include "SDLPlayback_p.h"

#include <QDebug>

static const ushort PLAYBACK_SAMPLE_FORMAT = AUDIO_F32SYS; // 32位浮点

static const ushort PLAYBACK_BUFFER_SAMPLES = 1024; // 默认缓冲区长度

static const ushort PLAYBACK_SAMPLE_RATE = 44100; // 默认采样率

static const uint PLAYBACK_POLL_INTERVAL_PLAYING = 1; // 播放轮循时间间隔(ms)

static const uint PLAYBACK_POLL_INTERVAL_IDLE = 1; // 空闲轮循时间间隔(ms)

static void workCallback(void *udata, quint8 *stream, int len) {
    auto d = (SDLPlaybackPrivate *) udata;
    d->workCallback(stream, len);
}

SDLPlaybackPrivate::SDLPlaybackPrivate() {
}

SDLPlaybackPrivate::~SDLPlaybackPrivate() {
}

void SDLPlaybackPrivate::init() {
    curDevId = 0;

    memset(&spec, 0, sizeof(spec));

    producer = nullptr;

    memset(&scb, 0, sizeof(scb));

    pcm_buffer_size = 0;
}

bool SDLPlaybackPrivate::setup(const QVariantMap &args) {
    Q_Q(SDLPlayback);

    QSignalBlocker blocker(q);

    // Init SDL
    SDL_LogSetPriority((int) SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    // 初始化
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        qDebug().noquote() << QString("SDL: Failed to initialize SDL: %1").arg(SDL_GetError());
        return false;
    }

#ifdef Q_OS_WINDOWS
    if (!switchDriver("directsound")) {
        return false;
    }
#endif

    bufferSamples = (bufferSamples <= 0) ? PLAYBACK_BUFFER_SAMPLES : bufferSamples;
    sampleRate = (sampleRate <= 0) ? PLAYBACK_SAMPLE_RATE : sampleRate;

    // 配置音频参数结构体
    spec.freq = sampleRate;
    spec.format = PLAYBACK_SAMPLE_FORMAT;
    spec.channels = channels;
    spec.silence = 0;
    spec.samples = bufferSamples; //缓冲区字节数/单个采样字节数/声道数
    spec.callback = ::workCallback;
    spec.userdata = (void *) this; // 回调到成员函数

    // 缓冲区
    pcm_buffer_size = bufferSamples * channels;
    pcm_buffer.reset(new float[pcm_buffer_size * sizeof(float)]);

    // 开始侦听音频设备
    producer = new std::thread(&SDLPlaybackPrivate::poll, this);

    Q_UNUSED(args);

    return true;
}

void SDLPlaybackPrivate::dispose() {
    Q_Q(SDLPlayback);

    QSignalBlocker blocker(q);

    // 结束侦听音频设备
    {
        notifyQuitPoll();
        producer->join();

        delete producer;
        producer = nullptr;
    }

    switchDevId(QString());
    switchDriver(QString());

    SDL_Quit();
}

void SDLPlaybackPrivate::play() {
    Q_Q(SDLPlayback);

    // 如果没有打开音频设备那么打开第一个音频设备
    if (curDevId == 0) {
        QStringList devices = q->devices();
        q->setDevice(devices.front());
    }

    // 初始化控制块
    scb.audio_chunk = nullptr;
    scb.audio_len = 0;
    scb.audio_pos = 0;

    notifyPlay();
    while (state == IAudioPlayback::Stopped) {
    }
}

void SDLPlaybackPrivate::stop() {
    notifyStop();
    while (state == IAudioPlayback::Playing) {
    }
}

void SDLPlaybackPrivate::switchState(IAudioPlayback::PlaybackState newState) {
    Q_Q(SDLPlayback);

    auto orgState = state;
    state = newState;

    // 通知播放状态已更改
    if (state != orgState) {
        emit q->stateChanged();
    }
}

bool SDLPlaybackPrivate::switchDevId(const QString &dev) {
    Q_Q(SDLPlayback);

    uint id = 0;
    if (!dev.isEmpty()) {
        // 打开音频设备
        if ((id = SDL_OpenAudioDevice(dev.toStdString().data(), 0, &spec, nullptr, 0)) == 0) {
            qDebug().noquote()
                << QString("SDLPlayback: Failed to open audio device: %1.").arg(SDL_GetError());
            return false;
        }
        qDebug().noquote() << QString("SDLPlayback: %1").arg(dev);
    }

    // 关闭旧音频设备
    if (curDevId > 0) {
        SDL_CloseAudioDevice(curDevId);
    }
    curDevId = id;

    auto orgDev = device;
    device = dev;

    // 通知音频设备已更改
    if (device != orgDev) {
        emit q->deviceChanged();
    }

    return true;
}

bool SDLPlaybackPrivate::switchDriver(const QString &drv) {
    Q_Q(SDLPlayback);

    switchDevId(QString());

    if (!driver.isEmpty()) {
        SDL_AudioQuit();
    }
    if (!drv.isEmpty()) {
        if (SDL_AudioInit(drv.toStdString().data()) < 0) {
            qDebug().noquote().noquote()
                << QString("SDLHost: Failed to initialize SDL: %1.").arg(SDL_GetError());
            return false;
        }
    }

    auto orgDriver = driver;
    driver = drv;

    if (driver != orgDriver) {
        emit q->driverChanged();
    }

    return true;
}

void SDLPlaybackPrivate::notifyGetAudioFrame() {
    SDL_Event e;
    e.type = (SDL_EventType) SDL_EVENT_BUFFER_END;
    SDL_PushEvent(&e);
}

void SDLPlaybackPrivate::notifyStop() {
    SDL_Event e;
    e.type = (SDL_EventType) SDL_EVENT_MANUAL_STOP;
    SDL_PushEvent(&e);
}

void SDLPlaybackPrivate::notifyPlay() {
    SDL_Event e;
    e.type = (SDL_EventType) SDL_EVENT_MANUAL_PLAY;
    SDL_PushEvent(&e);
}

void SDLPlaybackPrivate::notifyQuitPoll() {
    SDL_Event e;
    e.type = (SDL_EventType) SDL_EVENT_QUIT_POLL;
    SDL_PushEvent(&e);
}

void SDLPlaybackPrivate::workCallback(quint8 *stream, int len) {
    // 上锁
    mutex.lock();

    // 缓冲区置为静音
    SDL_memset(stream, 0, len);

    if (scb.audio_len > 0 && scb.audio_chunk) {
        len = qMin(len, scb.audio_len);

        // 将缓冲区中的声音写入流
        SDL_MixAudioFormat(stream, (quint8 *) (scb.audio_chunk + scb.audio_pos), spec.format,
                           (uint) len, SDL_MIX_MAXVOLUME);

        scb.audio_pos += len;
        scb.audio_len -= len;

        // 判断是否完毕
        if (scb.audio_len == 0) {
            notifyGetAudioFrame();
        }
    }

    // 放锁
    mutex.unlock();
}

void SDLPlaybackPrivate::poll() {
    Q_Q(SDLPlayback);

    bool quit = false;

    // 外层循环
    while (true) {
        bool over = false; // 每次循环末尾检测是否需要变更播放状态

        // 不停地获取事件
        SDL_Event e;
        while (SDL_PollEvent(&e) > 0) {
            switch ((int) e.type) {
                // 缓存用完
                case SDL_EVENT_BUFFER_END: {
                    // 上锁
                    mutex.lock();

                    float *pcm_buffer = this->pcm_buffer.data();

                    // 从文件中读取数据，剩下的就交给音频设备去完成了
                    // 它播放完一段数据后会执行回调函数，获取等多的数据
                    int samples = decoder->Read(pcm_buffer, 0, spec.samples * spec.channels);
                    if (samples <= 0) {
                        // 播放完毕
                        over = true;
                    } else {
                        // 重置缓冲区
                        scb.audio_chunk = (char *) pcm_buffer;
                        scb.audio_len =
                            samples * 4; // 长度为读出数据长度，在read_audio_data中做减法
                        scb.audio_pos = 0; // 设置当前位置为缓冲区头部
                    }

                    // 放锁
                    mutex.unlock();
                    break;
                }

                // 停止命令
                case SDL_EVENT_MANUAL_STOP: {
                    over = true;
                    break;
                }

                // 播放命令
                case SDL_EVENT_MANUAL_PLAY: {
                    // 设置暂停标识位
                    SDL_PauseAudioDevice(curDevId, 0);
                    switchState(IAudioPlayback::Playing);

                    // 第一次事件
                    notifyGetAudioFrame();
                    break;
                }

                // 结束命令
                case SDL_EVENT_QUIT_POLL: {
                    over = true;
                    quit = true;
                    break;
                }

                // 音频设备移除
                case SDL_AUDIODEVICEREMOVED: {
                    emit q->deviceRemoved();

                    quint32 dev = e.adevice.which;
                    if (dev == curDevId) {
                        switchDevId(QString());
                        over = true;
                    }

                    break;
                }

                // 音频设备添加
                case SDL_AUDIODEVICEADDED: {
                    emit q->deviceAdded();
                    break;
                }

                default:
                    break;
            }
        }

        if (over && state == IAudioPlayback::Playing) {
            // 设置暂停标识位
            SDL_PauseAudioDevice(curDevId, 1);
            switchState(IAudioPlayback::Stopped);
        }

        if (quit) {
            break;
        }

        SDL_Delay((state == IAudioPlayback::Playing) ? PLAYBACK_POLL_INTERVAL_PLAYING
                                                     : PLAYBACK_POLL_INTERVAL_IDLE);
    }
}
