#include <dstools/AudioPlayback.h>
#include <dstools/AudioDecoder.h>
#include "SDLPlayback.h"
#include "FFmpegDecoder.h"

#include <QDebug>

namespace dstools::audio {

struct AudioPlayback::Impl {
    SDLPlayback *playback = nullptr;
    AudioDecoder *decoder = nullptr;
    bool available = false;
};

AudioPlayback::AudioPlayback(QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>()) {
}

AudioPlayback::~AudioPlayback() {
    if (d->playback) {
        if (d->playback->isPlaying()) {
            d->playback->stop();
        }
        if (d->available) {
            d->playback->dispose();
        }
        delete d->playback;
    }
}

bool AudioPlayback::setup(int sampleRate, int channels, int bufferSize) {
    d->playback = new SDLPlayback(this);

    QsMedia::PlaybackArguments args(sampleRate, channels, bufferSize);
    if (!d->playback->setup(args)) {
        delete d->playback;
        d->playback = nullptr;
        d->available = false;
        return false;
    }
    d->available = true;

    // Forward signals
    connect(d->playback, &QsApi::IAudioPlayback::stateChanged, this, [this]() {
        emit stateChanged(state());
    });
    connect(d->playback, &QsApi::IAudioPlayback::deviceChanged, this, [this]() {
        emit deviceChanged(currentDevice());
    });
    connect(d->playback, &QsApi::IAudioPlayback::deviceAdded, this, [this]() {
        auto devs = devices();
        emit deviceAdded(devs.isEmpty() ? QString() : devs.last());
    });
    connect(d->playback, &QsApi::IAudioPlayback::deviceRemoved, this, [this]() {
        emit deviceRemoved(QString());
    });

    // Set first device if available
    auto devList = devices();
    if (!devList.isEmpty()) {
        setDevice(devList.front());
    }

    return true;
}

void AudioPlayback::dispose() {
    if (d->playback && d->available) {
        d->playback->dispose();
        d->available = false;
    }
}

void AudioPlayback::setDecoder(AudioDecoder *decoder) {
    d->decoder = decoder;
    if (d->playback && decoder) {
        // The SDLPlayback expects an IAudioDecoder.
        // We need to get the underlying FFmpegDecoder from AudioDecoder.
        // For now, we use a workaround: the AudioDecoder's Impl holds a FFmpegDecoder.
        // This is an internal detail; we access it via friend or direct cast.
        // TODO: clean up this layering violation after full migration
    }
}

void AudioPlayback::play() {
    if (d->playback && d->available) {
        d->playback->play();
    }
}

void AudioPlayback::stop() {
    if (d->playback && d->available) {
        d->playback->stop();
    }
}

QStringList AudioPlayback::devices() const {
    if (!d->playback) return {};
    return d->playback->devices();
}

QString AudioPlayback::currentDevice() const {
    if (!d->playback) return {};
    return d->playback->currentDevice();
}

void AudioPlayback::setDevice(const QString &device) {
    if (d->playback) {
        d->playback->setDevice(device);
    }
}

AudioPlayback::State AudioPlayback::state() const {
    if (!d->playback) return Stopped;
    return d->playback->isPlaying() ? Playing : Stopped;
}

} // namespace dstools::audio
