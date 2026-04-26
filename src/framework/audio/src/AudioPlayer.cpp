#include <dstools/AudioPlayer.h>
#include <dstools/AudioDecoder.h>
#include <dstools/AudioPlayback.h>
#include <dstools/WaveFormat.h>

namespace dstools::audio {

AudioPlayer::AudioPlayer(QObject *parent) : IAudioPlayer(parent) {
    m_playback = new AudioPlayback(this);

    if (!m_playback->setup(44100, 2, 1024)) {
        delete m_playback;
        m_playback = nullptr;
        m_valid = false;
        return;
    }
    // AudioPlayback::setDecoder takes ownership of the decoder.
    // All subsequent decoder access MUST go through m_playback's thread-safe
    // accessors to avoid data races with the SDL audio callback thread.
    m_playback->setDecoder(new AudioDecoder());
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
    m_playback->closeFile();
    return m_playback->openFile(path);
}

void AudioPlayer::close() {
    if (!m_valid) return;
    m_playback->stop();
    m_playback->closeFile();
}

bool AudioPlayer::isOpen() const {
    return m_valid && m_playback && m_playback->decoderIsOpen();
}

double AudioPlayer::duration() const {
    if (!m_valid || !m_playback || !m_playback->decoderIsOpen()) return 0.0;
    auto fmt = m_playback->decoderFormat();
    if (fmt.averageBytesPerSecond() <= 0) return 0.0;
    return static_cast<double>(m_playback->decoderLength()) / fmt.averageBytesPerSecond();
}

double AudioPlayer::position() const {
    if (!m_valid || !m_playback || !m_playback->decoderIsOpen()) return 0.0;
    auto fmt = m_playback->decoderFormat();
    if (fmt.averageBytesPerSecond() <= 0) return 0.0;
    return static_cast<double>(m_playback->decoderPosition()) / fmt.averageBytesPerSecond();
}

void AudioPlayer::setPosition(double sec) {
    if (!m_valid || !m_playback || !m_playback->decoderIsOpen()) return;
    auto fmt = m_playback->decoderFormat();
    if (fmt.averageBytesPerSecond() <= 0) return;
    qint64 pos = static_cast<qint64>(sec * fmt.averageBytesPerSecond());
    m_playback->decoderSetPosition(pos);
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
