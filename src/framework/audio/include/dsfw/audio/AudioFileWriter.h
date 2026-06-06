#pragma once
/// @file AudioFileWriter.h
/// @brief FFmpeg-based audio file writer (PIMPL).

#include <dsfw/audio/AudioBuffer.h>
#include <dsfw/Result.h>
#include <memory>
#include <string>

namespace dsfw::audio {

    /// @brief Write configuration for audio file output.
    struct WriteConfig {
        int sampleRate = 44100;
        int channelCount = 1;
        SampleFormat format = SampleFormat::Float32;

        /// Output container format hint ("wav", "flac", "ogg").
        /// Empty = auto-detect from file extension.
        std::string containerHint;
    };

    /// @brief FFmpeg-based audio file writer.
    ///
    /// Uses FFmpeg internally via PIMPL (INFRA-13).
    /// Supports writing float32/int16 PCM data to WAV and other formats.
    class AudioFileWriter {
    public:
        AudioFileWriter();
        ~AudioFileWriter();

        // Non-copyable, movable
        AudioFileWriter(const AudioFileWriter &) = delete;
        AudioFileWriter &operator=(const AudioFileWriter &) = delete;
        AudioFileWriter(AudioFileWriter &&) noexcept;
        AudioFileWriter &operator=(AudioFileWriter &&) noexcept;

        /// @brief Open a file for writing.
        /// @param path UTF-8 file path.
        /// @param config Write configuration.
        dsfw::Result<void> open(const std::string &path, const WriteConfig &config);

        /// @brief Write interleaved float32 samples.
        ///        Must match channelCount from WriteConfig.
        dsfw::Result<void> writeFloats(const float *samples, int64_t frameCount);

        /// @brief Write interleaved int16 samples.
        ///        Must match channelCount from WriteConfig.
        dsfw::Result<void> writeInt16s(const int16_t *samples, int64_t frameCount);

        /// @brief Write an AudioBuffer.
        dsfw::Result<void> writeBuffer(const AudioBuffer &buffer);

        /// @brief Close the file and finalize the header.
        ///        Called automatically by destructor if not called explicitly.
        void close();

        [[nodiscard]] bool isOpen() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> d;
    };

    /// @brief Convenience: write a complete AudioBuffer to a file in one call.
    dsfw::Result<void> writeAudioFile(const std::string &path, const AudioBuffer &buffer,
                                       int sampleRate, const std::string &containerHint = {});

} // namespace dsfw::audio
