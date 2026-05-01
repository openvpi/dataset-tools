#pragma once
/// @file IAudioPlayer.h
/// @brief Abstract interface for audio player implementations.

#include <QObject>
#include <QStringList>
#include <memory>

namespace dstools::audio {

/// @brief Abstract audio player interface.
class IAudioPlayer : public QObject {
    Q_OBJECT
public:
    explicit IAudioPlayer(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IAudioPlayer() = default;

    /// @brief Open an audio file.
    /// @param path Path to the audio file.
    /// @return True on success.
    virtual bool open(const QString &path) = 0;

    /// @brief Close the current audio file.
    virtual void close() = 0;

    /// @brief Check whether a file is open.
    /// @return True if a file is open.
    virtual bool isOpen() const = 0;

    /// @brief Get total duration in seconds.
    /// @return Duration in seconds.
    virtual double duration() const = 0;

    /// @brief Get the current playback position in seconds.
    /// @return Position in seconds.
    virtual double position() const = 0;

    /// @brief Seek to a position in seconds.
    /// @param sec Target position.
    virtual void setPosition(double sec) = 0;

    /// @brief Start playback.
    virtual void play() = 0;

    /// @brief Stop playback.
    virtual void stop() = 0;

    /// @brief Check whether playback is active.
    /// @return True if playing.
    virtual bool isPlaying() const = 0;

    /// @brief Get available audio output devices.
    /// @return List of device names.
    virtual QStringList devices() const = 0;

    /// @brief Get the current output device name.
    /// @return Current device name.
    virtual QString currentDevice() const = 0;

    /// @brief Set the audio output device.
    /// @param device Device name.
    virtual void setDevice(const QString &device) = 0;

    /// @brief Initialize the audio subsystem.
    /// @param sampleRate Output sample rate in Hz.
    /// @param channels Number of output channels.
    /// @param bufferSize Audio buffer size in samples.
    /// @return True on success.
    virtual bool setup(int sampleRate, int channels, int bufferSize) = 0;

signals:
    /// @brief Emitted when the playback state changes.
    void stateChanged();

    /// @brief Emitted when the output device changes.
    /// @param device New device name.
    void deviceChanged(const QString &device);
};

}
