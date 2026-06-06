#pragma once
/// @file FfmpegAudioDecoder.h
/// @brief FFmpeg-based audio decoder implementation (PIMPL).

#include <dsfw/audio/AudioBuffer.h>
#include <dsfw/audio/AudioFormatInfo.h>
#include <dsfw/Result.h>
#include <memory>
#include <string>

namespace dsfw::audio {

    /// @brief FFmpeg-based audio decoder.
    ///
    /// Uses FFmpeg internally via PIMPL (INFRA-13).
    /// Outputs source format (no built-in resampling) — use SwresampleResampler separately.
    class FfmpegAudioDecoder {
    public:
        FfmpegAudioDecoder();
        ~FfmpegAudioDecoder();

        // Non-copyable, movable
        FfmpegAudioDecoder(const FfmpegAudioDecoder &) = delete;
        FfmpegAudioDecoder &operator=(const FfmpegAudioDecoder &) = delete;
        FfmpegAudioDecoder(FfmpegAudioDecoder &&) noexcept;
        FfmpegAudioDecoder &operator=(FfmpegAudioDecoder &&) noexcept;

        dsfw::Result<AudioFormatInfo> probe(const std::string &path);
        dsfw::Result<void> open(const std::string &path);
        void close();
        [[nodiscard]] bool isOpen() const;
        [[nodiscard]] const AudioFormatInfo &formatInfo() const;
        dsfw::Result<AudioBuffer> decodeAll();
        dsfw::Result<AudioBuffer> decodeSegment(double startSec, double endSec);
        dsfw::Result<AudioBuffer> read(int64_t frameCount);
        dsfw::Result<void> seekToTime(double sec);
        [[nodiscard]] double currentTime() const;
        [[nodiscard]] double totalDuration() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> d;
    };

} // namespace dsfw::audio