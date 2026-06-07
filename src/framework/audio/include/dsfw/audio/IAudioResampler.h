#pragma once
/// @file IAudioResampler.h
/// @brief Abstract audio resampler interface (ARCH-02: depend on abstractions).

#include <dsfw/audio/AudioBuffer.h>
#include <dsfw/audio/ResampleConfig.h>
#include <dsfw/Result.h>
#include <memory>

namespace dsfw::audio {

    /// @brief Abstract audio resampler interface.
    ///
    /// AudioPipeline depends on this interface, allowing different
    /// resampler implementations (libswresample, mock, etc.) via constructor injection.
    class IAudioResampler {
    public:
        virtual ~IAudioResampler() = default;

        /// @brief Convert audio buffer to target format/rate/channels.
        virtual dsfw::Result<AudioBuffer> convert(const AudioBuffer &input, int inputSampleRate,
                                                     const ResampleConfig &config) = 0;
    };

} // namespace dsfw::audio