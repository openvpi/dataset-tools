#pragma once
/// @file AudioFormatInfo.h
/// @brief Audio format info obtained from probing without full decoding.

#include <dsfw/audio/AudioBuffer.h>
#include <string>

namespace dsfw::audio {

    /// @brief Audio format info obtained without full decoding.
    struct AudioFormatInfo {
        int sampleRate = 0;
        int channelCount = 0;
        SampleFormat sampleFormat = SampleFormat::Unknown;
        int64_t totalFrameCount = 0; ///< -1 if unknown (streaming)
        double durationSec = 0.0;
        int bitRate = 0;             ///< kbps, 0 if unknown
        std::string codecName;       ///< "pcm_s16le", "flac", "mp3", "vorbis", etc.
        std::string containerName;   ///< "wav", "flac", "mp3", "ogg", etc.
    };

} // namespace dsfw::audio