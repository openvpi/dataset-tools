#pragma once
/// @file AudioPlayback.h
/// @brief SDL2-based audio playback engine with device management.

#include <dstools/WaveFormat.h>
#include <QObject>
#include <QStringList>
#include <memory>

namespace dstools::audio {

class AudioDecoder;

/// @brief Audio playback engine using SDL2 internally.
///
/// Replaces original qsmedia::IAudioPlayback + SDLPlayback plugin.
/// Bug fixes vs original:
///   - std::atomic<int> replaces volatile int state (BUG-018)
///   - std::lock_guard replaces manual lock/unlock (BUG-019)
///   - Empty device list guard (BUG-020)
///   - pcm_buffer allocation fix (CQ-011)
class AudioPlayback : public QObject {
    Q_OBJECT
public:
    explicit AudioPlayback(QObject *parent = nullptr);
    ~AudioPlayback() override;

    AudioPlayback(const AudioPlayback &) = delete;
    AudioPlayback &operator=(const AudioPlayback &) = delete;

    /// @brief Initialize the audio subsystem.
    /// @param sampleRate Output sample rate in Hz.
    /// @param channels Number of output channels.
    /// @param bufferSize Audio buffer size in samples.
    /// @return False if initialization failed (all playback methods become no-ops).
    bool setup(int sampleRate = 44100, int channels = 2, int bufferSize = 1024);

    /// @brief Release the audio subsystem.
    void dispose();

    /// @brief Set the decoder source (takes ownership).
    /// @param decoder Audio decoder to use for PCM data.
    void setDecoder(AudioDecoder *decoder);

    /// @brief Open an audio file using an internal decoder.
    /// @param path Path to the audio file.
    /// @return True on success.
    bool openFile(const QString &path);

    /// @brief Close the currently open audio file.
    void closeFile();

    /// @brief Start playback.
    void play();

    /// @brief Stop playback.
    void stop();

    /// @brief Get the list of available audio output devices.
    /// @return Device name list.
    QStringList devices() const;

    /// @brief Get the name of the current output device.
    /// @return Current device name.
    QString currentDevice() const;

    /// @brief Set the audio output device.
    /// @param device Device name to switch to.
    void setDevice(const QString &device);

    /// @brief Playback state.
    enum State { Stopped, Playing };

    /// @brief Get the current playback state.
    /// @return Current state.
    State state() const;

    /// @brief Set the decoder read position (thread-safe).
    /// @param pos Byte offset to seek to.
    void decoderSetPosition(qint64 pos);

    /// @brief Get the decoder read position (thread-safe).
    /// @return Current byte offset.
    qint64 decoderPosition() const;

    /// @brief Get the total decoder data length (thread-safe).
    /// @return Total length in bytes.
    qint64 decoderLength() const;

    /// @brief Check whether the decoder has an open file (thread-safe).
    /// @return True if decoder is open.
    bool decoderIsOpen() const;

    /// @brief Get the decoder audio format (thread-safe).
    /// @return Audio format descriptor.
    WaveFormat decoderFormat() const;

signals:
    /// @brief Emitted when the playback state changes.
    /// @param newState The new playback state.
    void stateChanged(AudioPlayback::State newState);

    /// @brief Emitted when the output device changes.
    /// @param device The new device name.
    void deviceChanged(const QString &device);

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace dstools::audio
