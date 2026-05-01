#pragma once
#include <dstools/WaveFormat.h>
#include <QString>
#include <memory>

namespace dstools::audio {

/// Audio decoder. Uses FFmpeg internally.
/// Replaces original qsmedia::IAudioDecoder + FFmpegDecoder plugin.
class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();

    AudioDecoder(const AudioDecoder &) = delete;
    AudioDecoder &operator=(const AudioDecoder &) = delete;

    /// Open an audio file (supports wav/mp3/m4a/flac)
    bool open(const QString &path);

    /// Close current file
    void close();

    /// Whether a file is currently open
    bool isOpen() const;

    /// Get audio format
    WaveFormat format() const;

    /// Read PCM data into buffer+offset, up to count bytes.
    /// Returns actual bytes read, 0 means EOF.
    int read(char *buffer, int offset, int count);

    /// Read PCM data as float samples
    int read(float *buffer, int offset, int count);

    /// Set playback position (byte offset)
    void setPosition(qint64 pos);

    /// Get current position (byte offset)
    qint64 position() const;

    /// Get total length (bytes)
    qint64 length() const;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace dstools::audio
