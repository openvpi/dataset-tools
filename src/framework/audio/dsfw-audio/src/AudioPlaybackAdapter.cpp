#include <dsfw/audio/AudioPlaybackAdapter.h>

#include <SDL2/SDL.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <mutex>
#include <vector>

namespace dsfw::audio {

struct AudioPlaybackAdapter::Impl {
    // SDL state
    SDL_AudioDeviceID deviceId = 0;
    bool sdlInitialized = false;
    bool available = false;

    // Audio format
    int sampleRate = 44100;
    int channels = 2;
    int bufferSize = 1024;

    // Playback state
    std::atomic<bool> playing{false};
    std::string currentDeviceName;

    // Audio data (thread-safe access via mutex)
    mutable std::mutex dataMutex;
    std::vector<float> pcmData;
    int64_t totalFrames = 0;
    int dataSampleRate = 44100;
    int dataChannels = 2;
    int64_t readPos = 0; // in frames

    // Callbacks
    StateCallback stateCallback;
    DeviceCallback deviceCallback;

    static void audioCallback(void *userdata, Uint8 *stream, int len);
};

void AudioPlaybackAdapter::Impl::audioCallback(void *userdata, Uint8 *stream, int len) {
    auto *impl = static_cast<Impl *>(userdata);
    std::lock_guard lock(impl->dataMutex);

    if (!impl->playing.load() || impl->pcmData.empty()) {
        std::memset(stream, 0, len);
        return;
    }

    // len is in bytes, we need to convert to frames
    int bytesPerFrame = impl->dataChannels * static_cast<int>(sizeof(float));
    int requestedFrames = len / bytesPerFrame;
    int64_t remainingFrames = impl->totalFrames - impl->readPos;
    int framesToCopy = std::min(requestedFrames, static_cast<int>(remainingFrames));

    if (framesToCopy > 0) {
        int bytesToCopy = framesToCopy * bytesPerFrame;
        std::memcpy(stream, impl->pcmData.data() + impl->readPos * impl->dataChannels, bytesToCopy);
        impl->readPos += framesToCopy;
    }

    if (framesToCopy < requestedFrames) {
        std::memset(stream + framesToCopy * bytesPerFrame, 0, len - framesToCopy * bytesPerFrame);
        impl->playing.store(false);
    }
}

// ── PIMPL forwarding ────────────────────────────────────────────────────────

AudioPlaybackAdapter::AudioPlaybackAdapter() : d(std::make_unique<Impl>()) {}

AudioPlaybackAdapter::~AudioPlaybackAdapter() {
    dispose();
}

AudioPlaybackAdapter::AudioPlaybackAdapter(AudioPlaybackAdapter &&) noexcept = default;
AudioPlaybackAdapter &AudioPlaybackAdapter::operator=(AudioPlaybackAdapter &&) noexcept = default;

bool AudioPlaybackAdapter::setup(int sampleRate, int channels, int bufferSize) {
    if (!d->sdlInitialized) {
        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
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

void AudioPlaybackAdapter::dispose() {
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

bool AudioPlaybackAdapter::isSetup() const {
    return d->available;
}

void AudioPlaybackAdapter::setAudioData(const float *data, int64_t frameCount, int sampleRate, int channels) {
    std::lock_guard lock(d->dataMutex);
    d->pcmData.assign(data, data + frameCount * channels);
    d->totalFrames = frameCount;
    d->dataSampleRate = sampleRate;
    d->dataChannels = channels;
    d->readPos = 0;
}

void AudioPlaybackAdapter::clearAudioData() {
    std::lock_guard lock(d->dataMutex);
    d->pcmData.clear();
    d->totalFrames = 0;
    d->readPos = 0;
}

void AudioPlaybackAdapter::play() {
    if (!d->available || d->pcmData.empty()) return;

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
        const char *devName = d->currentDeviceName.empty() ? nullptr : d->currentDeviceName.c_str();
        d->deviceId = SDL_OpenAudioDevice(devName, 0, &desired, &obtained, 0);
        if (!d->deviceId) {
            return;
        }
    }

    bool wasPlaying = d->playing.exchange(true);
    SDL_PauseAudioDevice(d->deviceId, 0);

    if (!wasPlaying && d->stateCallback) {
        d->stateCallback(true);
    }
}

void AudioPlaybackAdapter::stop() {
    if (!d->available) return;

    bool wasPlaying = d->playing.exchange(false);

    if (d->deviceId) {
        SDL_PauseAudioDevice(d->deviceId, 1);
    }

    if (wasPlaying && d->stateCallback) {
        d->stateCallback(false);
    }
}

bool AudioPlaybackAdapter::isPlaying() const {
    return d->playing.load();
}

void AudioPlaybackAdapter::setPosition(double sec) {
    std::lock_guard lock(d->dataMutex);
    int64_t pos = static_cast<int64_t>(sec * d->dataSampleRate);
    d->readPos = std::max<int64_t>(0, std::min(pos, d->totalFrames));
}

double AudioPlaybackAdapter::currentTime() const {
    std::lock_guard lock(d->dataMutex);
    if (d->dataSampleRate <= 0) return 0.0;
    return static_cast<double>(d->readPos) / d->dataSampleRate;
}

double AudioPlaybackAdapter::totalDuration() const {
    std::lock_guard lock(d->dataMutex);
    if (d->dataSampleRate <= 0) return 0.0;
    return static_cast<double>(d->totalFrames) / d->dataSampleRate;
}

std::vector<std::string> AudioPlaybackAdapter::devices() const {
    std::vector<std::string> result;
    if (!d->sdlInitialized) return result;

    int count = SDL_GetNumAudioDevices(0);
    for (int i = 0; i < count; ++i) {
        const char *name = SDL_GetAudioDeviceName(i, 0);
        if (name) result.emplace_back(name);
    }
    return result;
}

std::string AudioPlaybackAdapter::currentDevice() const {
    return d->currentDeviceName;
}

void AudioPlaybackAdapter::setDevice(const std::string &device) {
    if (d->currentDeviceName == device) return;

    bool wasPlaying = d->playing.load();
    if (wasPlaying) stop();

    if (d->deviceId) {
        SDL_CloseAudioDevice(d->deviceId);
        d->deviceId = 0;
    }

    d->currentDeviceName = device;

    if (d->deviceCallback) {
        d->deviceCallback(device);
    }

    if (wasPlaying) play();
}

void AudioPlaybackAdapter::setStateCallback(StateCallback cb) {
    d->stateCallback = std::move(cb);
}

void AudioPlaybackAdapter::setDeviceCallback(DeviceCallback cb) {
    d->deviceCallback = std::move(cb);
}

} // namespace dsfw::audio