#pragma once
/// @file AudioDecoder.h
/// @brief FFmpeg-based audio decoder for reading PCM data from audio files.

#include <dstools/WaveFormat.h>
#include <QString>
#include <memory>

namespace dstools::audio {

/// @brief Audio decoder using FFmpeg internally.
///
/// Replaces original qsmedia::IAudioDecoder + FFmpegDecoder plugin.
class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();

    AudioDecoder(const AudioDecoder &) = delete;
    AudioDecoder &operator=(const AudioDecoder &) = delete;

    /// @brief Open an audio file.
    /// @param path Path to audio file (supports wav/mp3/m4a/flac).
    /// @return True on success.
    bool open(const QString &path);

    /// @brief Close the current file.
    void close();

    /// @brief Check whether a file is currently open.
    /// @return True if a file is open.
    bool isOpen() const;

    /// @brief Get the audio format of the opened file.
    /// @return Audio format descriptor.
    WaveFormat format() const;

    /// @brief Read PCM data into a byte buffer.
    /// @param buffer Destination buffer.
    /// @param offset Byte offset into the buffer.
    /// @param count Maximum number of bytes to read.
    /// @return Actual bytes read; 0 means EOF.
    int read(char *buffer, int offset, int count);

    /// @brief Read PCM data as float samples.
    /// @param buffer Destination buffer.
    /// @param offset Sample offset into the buffer.
    /// @param count Maximum number of samples to read.
    /// @return Actual samples read; 0 means EOF.
    int read(float *buffer, int offset, int count);

    /// @brief Set the playback position.
    /// @param pos Byte offset to seek to.
    void setPosition(qint64 pos);

    /// @brief Get the current read position.
    /// @return Current byte offset.
    qint64 position() const;

    /// @brief Get the total length of the audio data.
    /// @return Total length in bytes.
    qint64 length() const;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace dstools::audio
