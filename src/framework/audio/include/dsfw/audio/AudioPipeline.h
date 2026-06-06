#pragma once
/// @file AudioPipeline.h
/// @brief Convenience pipeline combining decoder + resampler.

#include <dsfw/audio/AudioBuffer.h>
#include <dsfw/audio/AudioFormatInfo.h>
#include <dsfw/audio/FfmpegAudioDecoder.h>
#include <dsfw/audio/SwresampleResampler.h>
#include <dsfw/audio/ResampleConfig.h>
#include <dsfw/Result.h>
#include <memory>
#include <string>

namespace dsfw::audio {

    /// @brief Convenience pipeline: decode + resample in one call.
    ///
    /// Streams through decoder→resampler to keep memory low.
    class AudioPipeline {
    public:
        /// @brief Create with default decoder and resampler.
        static AudioPipeline create();

        /// @brief Create with custom decoder and resampler (for testing).
        AudioPipeline(std::unique_ptr<FfmpegAudioDecoder> decoder, std::unique_ptr<SwresampleResampler> resampler);

        /// @brief Probe a file without decoding.
        dsfw::Result<AudioFormatInfo> probe(const std::string &path);

        /// @brief Decode a file and resample to target format.
        ///        Streams through decoder→resampler to keep memory low.
        dsfw::Result<AudioBuffer> decodeAndResample(const std::string &path, const ResampleConfig &config);

        /// @brief Decode a time segment and resample.
        dsfw::Result<AudioBuffer> decodeSegmentAndResample(const std::string &path, double startSec,
                                                              double endSec, const ResampleConfig &config);

        /// @brief Decode entire file to mono float32 at target rate.
        ///        Convenience for model inference pipelines.
        dsfw::Result<AudioBuffer> decodeToMonoFloat(const std::string &path, int targetSampleRate = 16000);

        /// @brief Access the underlying decoder.
        [[nodiscard]] FfmpegAudioDecoder *decoder() const { return m_decoder.get(); }

        /// @brief Access the underlying resampler.
        [[nodiscard]] SwresampleResampler *resampler() const { return m_resampler.get(); }

    private:
        std::unique_ptr<FfmpegAudioDecoder> m_decoder;
        std::unique_ptr<SwresampleResampler> m_resampler;
    };

} // namespace dsfw::audio