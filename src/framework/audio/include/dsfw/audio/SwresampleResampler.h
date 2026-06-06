#pragma once
/// @file SwresampleResampler.h
/// @brief FFmpeg libswresample-based resampler implementation (PIMPL).

#include <dsfw/audio/AudioBuffer.h>
#include <dsfw/audio/ResampleConfig.h>
#include <dsfw/Result.h>
#include <memory>

namespace dsfw::audio {

    /// @brief Resampler using FFmpeg libswresample.
    ///
    /// Uses FFmpeg internally via PIMPL (INFRA-13).
    class SwresampleResampler {
    public:
        SwresampleResampler();
        ~SwresampleResampler();

        // Non-copyable, movable
        SwresampleResampler(const SwresampleResampler &) = delete;
        SwresampleResampler &operator=(const SwresampleResampler &) = delete;
        SwresampleResampler(SwresampleResampler &&) noexcept;
        SwresampleResampler &operator=(SwresampleResampler &&) noexcept;

        dsfw::Result<AudioBuffer> convert(const AudioBuffer &input, int inputSampleRate,
                                             const ResampleConfig &config);

        /// @brief Convenience: resample to mono float32 at target rate.
        dsfw::Result<AudioBuffer> toMonoFloat(const AudioBuffer &input, int inputSampleRate, int targetSampleRate) {
            auto config = ResampleConfig::forMonoFloat(targetSampleRate);
            return convert(input, inputSampleRate, config);
        }

    private:
        struct Impl;
        std::unique_ptr<Impl> d;
    };

} // namespace dsfw::audio