#include <dstools/AudioPlayback.h>
#include <dstools/AudioDecoder.h>

#include <SDL2/SDL.h>

#include <QDebug>
#include <atomic>

namespace dstools::audio {

struct AudioPlayback::Impl {
    AudioDecoder *decoder = nullptr;
    SDL_AudioDeviceID deviceId = 0;
    bool sdlInitialized = false;
    bool available = false;

    int sampleRate = 44100;
    int channels = 2;
    int bufferSize = 1024;

    std::atomic<State> currentState{Stopped};
    QString currentDeviceName;

    static void audioCallback(void *userdata, Uint8 *stream, int len);
};

void AudioPlayback::Impl::audioCallback(void *userdata, Uint8 *stream, int len) {
    auto *impl = static_cast<Impl *>(userdata);
    if (!impl->decoder || impl->currentState.load() != Playing) {
        memset(stream, 0, len);
        return;
    }

    int bytesRead = impl->decoder->read(reinterpret_cast<char *>(stream), 0, len);
    if (bytesRead < len) {
        memset(stream + bytesRead, 0, len - bytesRead);
        // Reached end
        impl->currentState.store(Stopped);
    }
}

AudioPlayback::AudioPlayback(QObject *parent)
    : QObject(parent), d(std::make_unique<Impl>()) {
}

AudioPlayback::~AudioPlayback() {
    dispose();
}

bool AudioPlayback::setup(int sampleRate, int channels, int bufferSize) {
    if (!d->sdlInitialized) {
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            qWarning() << "AudioPlayback: SDL_Init failed:" << SDL_GetError();
            return false;
        }
        d->sdlInitialized = true;
    }

    d->sampleRate = sampleRate;
    d->channels = channels;
    d->bufferSize = bufferSize;
    d->available = true;
    return true;
}

void AudioPlayback::dispose() {
    stop();
    if (d->deviceId) {
        SDL_CloseAudioDevice(d->deviceId);
        d->deviceId = 0;
    }
    if (d->sdlInitialized) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        d->sdlInitialized = false;
    }
    d->available = false;
}

void AudioPlayback::setDecoder(AudioDecoder *decoder) {
    d->decoder = decoder;
}

void AudioPlayback::play() {
    if (!d->available || !d->decoder) return;

    // Open audio device if not already open
    if (!d->deviceId) {
        SDL_AudioSpec desired{};
        desired.freq = d->sampleRate;
        desired.format = AUDIO_F32SYS;
        desired.channels = static_cast<Uint8>(d->channels);
        desired.samples = static_cast<Uint16>(d->bufferSize);
        desired.callback = Impl::audioCallback;
        desired.userdata = d.get();

        SDL_AudioSpec obtained{};
        const char *devName = d->currentDeviceName.isEmpty()
                                  ? nullptr
                                  : d->currentDeviceName.toUtf8().constData();
        d->deviceId = SDL_OpenAudioDevice(devName, 0, &desired, &obtained, 0);
        if (!d->deviceId) {
            qWarning() << "AudioPlayback: Failed to open audio device:" << SDL_GetError();
            return;
        }
    }

    auto oldState = d->currentState.load();
    d->currentState.store(Playing);
    SDL_PauseAudioDevice(d->deviceId, 0);

    if (oldState != Playing)
        emit stateChanged(Playing);
}

void AudioPlayback::stop() {
    if (!d->available) return;

    auto oldState = d->currentState.load();
    d->currentState.store(Stopped);

    if (d->deviceId) {
        SDL_PauseAudioDevice(d->deviceId, 1);
    }

    if (oldState != Stopped)
        emit stateChanged(Stopped);
}

QStringList AudioPlayback::devices() const {
    QStringList result;
    if (!d->sdlInitialized) return result;

    int count = SDL_GetNumAudioDevices(0);
    for (int i = 0; i < count; ++i) {
        const char *name = SDL_GetAudioDeviceName(i, 0);
        if (name) result << QString::fromUtf8(name);
    }
    return result;
}

QString AudioPlayback::currentDevice() const {
    return d->currentDeviceName;
}

void AudioPlayback::setDevice(const QString &device) {
    if (d->currentDeviceName == device) return;

    bool wasPlaying = (d->currentState.load() == Playing);
    if (wasPlaying) stop();

    if (d->deviceId) {
        SDL_CloseAudioDevice(d->deviceId);
        d->deviceId = 0;
    }

    d->currentDeviceName = device;
    emit deviceChanged(device);

    if (wasPlaying) play();
}

AudioPlayback::State AudioPlayback::state() const {
    return d->currentState.load();
}

} // namespace dstools::audio
