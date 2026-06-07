#pragma once
/// @file FfmpegUtils.h
/// @brief Shared FFmpeg utility helpers. Internal header, not for public API.

#include <dsfw/audio/AudioBuffer.h>

#include <string>

extern "C" {
#include <libavutil/error.h>
#include <libavutil/samplefmt.h>
}

namespace dsfw::audio::internal {

/// @brief Convert FFmpeg error code to human-readable string.
inline std::string ffmpegError(int err) {
    char buf[AV_ERROR_MAX_STRING_SIZE];
    return av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, err);
}

/// @brief Convert dsfw SampleFormat to FFmpeg AVSampleFormat.
inline AVSampleFormat toAVSampleFormat(SampleFormat fmt) {
    switch (fmt) {
    case SampleFormat::Float32:
        return AV_SAMPLE_FMT_FLT;
    case SampleFormat::Int16:
        return AV_SAMPLE_FMT_S16;
    case SampleFormat::Int32:
        return AV_SAMPLE_FMT_S32;
    default:
        return AV_SAMPLE_FMT_NONE;
    }
}

/// @brief Convert FFmpeg AVSampleFormat to dsfw SampleFormat.
inline SampleFormat fromAVSampleFormat(AVSampleFormat fmt) {
    switch (fmt) {
    case AV_SAMPLE_FMT_FLT:
    case AV_SAMPLE_FMT_FLTP:
        return SampleFormat::Float32;
    case AV_SAMPLE_FMT_S16:
    case AV_SAMPLE_FMT_S16P:
        return SampleFormat::Int16;
    case AV_SAMPLE_FMT_S32:
    case AV_SAMPLE_FMT_S32P:
        return SampleFormat::Int32;
    default:
        return SampleFormat::Unknown;
    }
}

}  // namespace dsfw::audio::internal