#pragma once

#include <filesystem>

#include <audio-util/AudioUtilGlobal.h>
#include <audio-util/SndfileVio.h>

namespace AudioUtil
{
    SF_VIO AUDIO_UTIL_EXPORT resample_to_vio(const std::filesystem::path &filepath, std::string &msg, int tar_channel,
                                             int tar_samplerate);
    bool AUDIO_UTIL_EXPORT write_vio_to_wav(SF_VIO &sf_vio_in, const std::filesystem::path &filepath,
                                            int tar_channel = -1);
} // namespace AudioUtil
