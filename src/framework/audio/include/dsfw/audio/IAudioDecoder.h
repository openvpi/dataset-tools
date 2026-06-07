#pragma once
/// @file IAudioDecoder.h
/// @brief Abstract audio decoder interface (ARCH-02: depend on abstractions).

#include <dsfw/audio/AudioBuffer.h>
#include <dsfw/audio/AudioFormatInfo.h>
#include <dsfw/Result.h>
#include <memory>
#include <string>

namespace dsfw::audio {

    /// @brief Abstract audio decoder interface.
    ///
    /// AudioPipeline depends on this interface, allowing different
    /// decoder implementations (FFmpeg, mock, etc.) via constructor injection.
    class IAudioDecoder {
    public:
        virtual ~IAudioDecoder() = default;

        /// @brief Probe a file without opening it.
        virtual dsfw::Result<AudioFormatInfo> probe(const std::string &path) = 0;

        /// @brief Open a file for reading.
        virtual dsfw::Result<void> open(const std::string &path) = 0;

        /// @brief Close the current file.
        virtual void close() = 0;

        /// @brief Read up to frameCount frames (streaming).
        virtual dsfw::Result<AudioBuffer> read(int64_t frameCount) = 0;

        /// @brief Seek to a time position in seconds.
        virtual dsfw::Result<void> seekToTime(double sec) = 0;
    };

} // namespace dsfw::audio