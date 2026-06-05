#pragma once
/// @file ResampleConfig.h
/// @brief Configuration for audio resampling.

#include <dsfw/audio/AudioBuffer.h>
#include <cstdint>

namespace dsfw::audio {

    /// @brief Resampling quality level.
    enum class ResampleQuality : uint8_t {
        Draft,  ///< Fast, lower quality (linear interpolation)
        Medium, ///< Balanced (default)
        High,   ///< High quality (soxr HQ equivalent)
        Ultra,  ///< Maximum quality (slowest)
    };

    /// @brief Downmix strategy for multi-channel to fewer channels.
    enum class DownmixStrategy : uint8_t {
        Average,     ///< Average all channels (default)
        Drop,        ///< Drop extra channels
        SelectFirst, ///< Keep only the first N channels
    };

    /// @brief Configuration for audio resampling.
    struct ResampleConfig {
        int targetSampleRate = 0;                         ///< 0 = keep original
        int targetChannelCount = 0;                       ///< 0 = keep original
        SampleFormat targetFormat = SampleFormat::Unknown; ///< Unknown = keep original
        ResampleQuality quality = ResampleQuality::High;
        DownmixStrategy downmix = DownmixStrategy::Average;

        /// @brief Convenience: create config for mono float32 at target rate.
        static ResampleConfig forMonoFloat(int sampleRate) {
            return {sampleRate, 1, SampleFormat::Float32, ResampleQuality::High, DownmixStrategy::Average};
        }

        /// @brief Convenience: create config for stereo float32 at target rate.
        static ResampleConfig forStereoFloat(int sampleRate) {
            return {sampleRate, 2, SampleFormat::Float32, ResampleQuality::High, DownmixStrategy::Average};
        }
    };

} // namespace dsfw::audio