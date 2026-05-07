#pragma once
/// @file AudioPlayer.h
/// @brief Concrete IAudioPlayer implementation combining AudioDecoder and AudioPlayback.

#include <dstools/IAudioPlayer.h>

namespace dstools::audio {

class AudioPlayback;

/// @brief Concrete audio player combining decoder and playback subsystems.
class AudioPlayer : public IAudioPlayer {
    Q_OBJECT
public:
    /// @brief Construct an AudioPlayer.
    /// @param parent Parent QObject.
    explicit AudioPlayer(QObject *parent = nullptr);
    ~AudioPlayer() override;

    /// @brief Open an audio file for playback.
    /// @param path Path to the audio file.
    /// @return True on success.
    bool open(const QString &path) override;

    /// @brief Close the current audio file.
    void close() override;

    /// @brief Check whether a file is open.
    /// @return True if a file is open.
    bool isOpen() const override;

    /// @brief Get the total duration in seconds.
    /// @return Duration in seconds.
    double duration() const override;

    /// @brief Get the current playback position in seconds.
    /// @return Position in seconds.
    double position() const override;

    /// @brief Seek to a position in seconds.
    /// @param sec Target position in seconds.
    void setPosition(double sec) override;

    /// @brief Start playback.
    void play() override;

    /// @brief Stop playback.
    void stop() override;

    /// @brief Check whether playback is active.
    /// @return True if playing.
    bool isPlaying() const override;

    /// @brief Get available audio output devices.
    /// @return List of device names.
    QStringList devices() const override;

    /// @brief Get the current output device name.
    /// @return Current device name.
    QString currentDevice() const override;

    /// @brief Set the audio output device.
    /// @param device Device name.
    void setDevice(const QString &device) override;

    /// @brief Initialize the audio subsystem.
    /// @param sampleRate Output sample rate in Hz.
    /// @param channels Number of output channels.
    /// @param bufferSize Audio buffer size in samples.
    /// @return True on success.
    bool setup(int sampleRate, int channels, int bufferSize) override;

private:
    AudioPlayback *m_playback = nullptr;
    bool m_valid = false;

    void onPlaybackStateChanged();
    void onDeviceChangedInternal();
};

}
