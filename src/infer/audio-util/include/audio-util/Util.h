/// @file Util.h
/// @brief Audio resampling and format conversion utilities.

#pragma once

#include <filesystem>

#include <audio-util/AudioUtilGlobal.h>
#include <audio-util/SndfileVio.h>

namespace AudioUtil
{
    /// @brief Resample an audio file and store the result in an in-memory VIO buffer.
    /// @param filepath Path to the input audio file.
    /// @param msg Output error message on failure.
    /// @param tar_channel Target number of channels.
    /// @param tar_samplerate Target sample rate in Hz.
    /// @return SF_VIO containing the resampled audio data.
    SF_VIO AUDIO_UTIL_EXPORT resample_to_vio(const std::filesystem::path &filepath, std::string &msg, int tar_channel,
                                             int tar_samplerate);

    /// @brief Write an in-memory VIO buffer to a WAV file.
    /// @param sf_vio_in Input VIO buffer with audio data.
    /// @param filepath Output WAV file path.
    /// @param tar_channel Target number of channels, or -1 to keep original.
    /// @return True on success.
    bool AUDIO_UTIL_EXPORT write_vio_to_wav(SF_VIO &sf_vio_in, const std::filesystem::path &filepath,
                                            int tar_channel = -1);
} // namespace AudioUtil
