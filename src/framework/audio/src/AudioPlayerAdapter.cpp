#include <dsfw/PathUtils.h>
#include <dsfw/audio/AudioPlayerAdapter.h>
#include <dsfw/audio/AudioPlaybackAdapter.h>
#include <dsfw/audio/AudioPipeline.h>
#include <dsfw/audio/FfmpegAudioDecoder.h>
#include <dsfw/audio/ResampleConfig.h>
#include <dsfw/audio/SwresampleResampler.h>

#include <QString>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <vector>

namespace dsfw::audio {

struct AudioPlayerAdapter::Impl {
    std::unique_ptr<AudioPlaybackAdapter> playback;
    std::unique_ptr<AudioPipeline> pipeline;

    bool fileOpen = false;
    std::vector<float> pcmData;
    int pcmSampleRate = 44100;
    int pcmChannels = 2;
    double totalDurationSec = 0.0;

    void onStateChanged(bool playing);
    void onDeviceChanged(const std::string &device);
};

void AudioPlayerAdapter::Impl::onStateChanged(bool playing) {
    // Bridged by AudioPlayerAdapter's private slot
}

void AudioPlayerAdapter::Impl::onDeviceChanged(const std::string &device) {
    // Bridged by AudioPlayerAdapter's private slot
}

AudioPlayerAdapter::AudioPlayerAdapter(QObject *parent)
    : IAudioPlayerAdapter(parent), d(std::make_unique<Impl>()) {
    d->playback = std::make_unique<AudioPlaybackAdapter>();
    d->pipeline = std::make_unique<AudioPipeline>(AudioPipeline::create());

    if (!d->playback->setup(44100, 2, 1024)) {
        d->playback.reset();
        return;
    }

    d->playback->setStateCallback([this](bool playing) {
        if (playing) {
            emit stateChanged(State::Playing);
        } else {
            emit stateChanged(State::Stopped);
        }
    });

    d->playback->setDeviceCallback([this](const std::string &device) {
        emit deviceChanged(QString::fromStdString(device));
    });
}

AudioPlayerAdapter::~AudioPlayerAdapter() {
    if (d->playback) {
        d->playback->stop();
        d->playback->dispose();
    }
}

dsfw::Result<void> AudioPlayerAdapter::open(const std::filesystem::path &path) {
    if (!d->playback || !d->pipeline) {
        return dsfw::Result<void>::Error("Audio player not initialized");
    }

    // Stop any current playback
    d->playback->stop();

    // Decode entire file to mono float32 at original rate first
    auto infoResult = d->pipeline->probe(PathUtils::toUtf8(path));
    if (!infoResult) {
        return dsfw::Result<void>::Error(infoResult.error());
    }
    auto &info = infoResult.value();

    // Resample to 44100Hz stereo float32 for playback
    ResampleConfig config;
    config.targetSampleRate = 44100;
    config.targetChannelCount = 2;
    config.targetFormat = SampleFormat::Float32;
    config.quality = ResampleQuality::High;

    auto decodeResult = d->pipeline->decodeAndResample(PathUtils::toUtf8(path), config);
    if (!decodeResult) {
        return dsfw::Result<void>::Error(decodeResult.error());
    }

    auto &buffer = decodeResult.value();
    int64_t totalFrames = buffer.frameCount();
    int channels = buffer.channelCount();

    // Copy float data
    auto floats = buffer.floats();
    d->pcmData.assign(floats.begin(), floats.end());
    d->pcmSampleRate = 44100;
    d->pcmChannels = channels;
    d->totalDurationSec = buffer.durationSec(44100);

    d->playback->setAudioData(d->pcmData.data(), totalFrames, d->pcmSampleRate, d->pcmChannels);
    d->fileOpen = true;

    return dsfw::Result<void>::Ok();
}

void AudioPlayerAdapter::close() {
    d->playback->stop();
    d->playback->clearAudioData();
    d->pcmData.clear();
    d->fileOpen = false;
    d->totalDurationSec = 0.0;
}

bool AudioPlayerAdapter::isOpen() const {
    return d->fileOpen;
}

double AudioPlayerAdapter::duration() const {
    return d->totalDurationSec;
}

double AudioPlayerAdapter::position() const {
    if (!d->playback) return 0.0;
    return d->playback->currentTime();
}

void AudioPlayerAdapter::setPosition(double sec) {
    if (!d->playback || !d->fileOpen) return;
    d->playback->setPosition(sec);
}

void AudioPlayerAdapter::play() {
    if (!d->playback || !d->fileOpen) return;
    d->playback->play();
}

void AudioPlayerAdapter::stop() {
    if (!d->playback) return;
    d->playback->stop();
}

bool AudioPlayerAdapter::isPlaying() const {
    if (!d->playback) return false;
    return d->playback->isPlaying();
}

QStringList AudioPlayerAdapter::devices() const {
    if (!d->playback) return {};
    QStringList result;
    for (const auto &dev : d->playback->devices()) {
        result << QString::fromStdString(dev);
    }
    return result;
}

QString AudioPlayerAdapter::currentDevice() const {
    if (!d->playback) return {};
    return QString::fromStdString(d->playback->currentDevice());
}

void AudioPlayerAdapter::setDevice(const QString &device) {
    if (!d->playback) return;
    d->playback->setDevice(device.toStdString());
}

bool AudioPlayerAdapter::setup(int sampleRate, int channels, int bufferSize) {
    if (!d->playback) return false;
    return d->playback->setup(sampleRate, channels, bufferSize);
}

} // namespace dsfw::audio