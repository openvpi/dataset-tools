#include <dstools/AudioPlayer.h>
#include <dstools/AudioDecoder.h>
#include <dstools/AudioPlayback.h>
#include <dstools/WaveFormat.h>

namespace dstools::audio {

AudioPlayer::AudioPlayer(QObject *parent) : IAudioPlayer(parent) {
    m_decoder = std::make_unique<AudioDecoder>();
    m_playback = new AudioPlayback(this);

    if (!m_playback->setup(44100, 2, 1024)) {
        delete m_playback;
        m_playback = nullptr;
        m_decoder.reset();
        m_valid = false;
        return;
    }
    m_playback->setDecoder(m_decoder.get());
    m_valid = true;

    connect(m_playback, &AudioPlayback::stateChanged,
            this, &AudioPlayer::onPlaybackStateChanged);
    connect(m_playback, &AudioPlayback::deviceChanged,
            this, &AudioPlayer::onDeviceChangedInternal);
}

AudioPlayer::~AudioPlayer() {
    if (m_playback) {
        m_playback->stop();
        m_playback->dispose();
    }
}

bool AudioPlayer::open(const QString &path) {
    if (!m_valid) return false;
    m_playback->stop();
    m_decoder->close();
    if (m_decoder->open(path)) {
        return true;
    }
    return false;
}

void AudioPlayer::close() {
    if (!m_valid) return;
    m_playback->stop();
    m_decoder->close();
}

bool AudioPlayer::isOpen() const {
    return m_valid && m_decoder && m_decoder->isOpen();
}

double AudioPlayer::duration() const {
    if (!m_valid || !m_decoder || !m_decoder->isOpen()) return 0.0;
    auto fmt = m_decoder->format();
    if (fmt.averageBytesPerSecond() <= 0) return 0.0;
    return static_cast<double>(m_decoder->length()) / fmt.averageBytesPerSecond();
}

double AudioPlayer::position() const {
    if (!m_valid || !m_decoder || !m_decoder->isOpen()) return 0.0;
    auto fmt = m_decoder->format();
    if (fmt.averageBytesPerSecond() <= 0) return 0.0;
    return static_cast<double>(m_decoder->position()) / fmt.averageBytesPerSecond();
}

void AudioPlayer::setPosition(double sec) {
    if (!m_valid || !m_decoder || !m_decoder->isOpen()) return;
    auto fmt = m_decoder->format();
    if (fmt.averageBytesPerSecond() <= 0) return;
    qint64 pos = static_cast<qint64>(sec * fmt.averageBytesPerSecond());
    m_decoder->setPosition(pos);
}

void AudioPlayer::play() {
    if (!m_valid || !m_playback) return;
    m_playback->play();
}

void AudioPlayer::stop() {
    if (!m_valid || !m_playback) return;
    m_playback->stop();
}

bool AudioPlayer::isPlaying() const {
    if (!m_valid || !m_playback) return false;
    return m_playback->state() == AudioPlayback::Playing;
}

QStringList AudioPlayer::devices() const {
    if (!m_playback) return {};
    return m_playback->devices();
}

QString AudioPlayer::currentDevice() const {
    if (!m_playback) return {};
    return m_playback->currentDevice();
}

void AudioPlayer::setDevice(const QString &device) {
    if (!m_playback) return;
    m_playback->setDevice(device);
}

bool AudioPlayer::setup(int sampleRate, int channels, int bufferSize) {
    if (m_playback) {
        return m_playback->setup(sampleRate, channels, bufferSize);
    }
    return false;
}

void AudioPlayer::onPlaybackStateChanged() {
    emit stateChanged();
}

void AudioPlayer::onDeviceChangedInternal() {
    if (m_playback)
        emit deviceChanged(m_playback->currentDevice());
}

}
