/// @file FlacDecoder.h
/// @brief FLAC to WAV decoder writing to in-memory VIO.

#pragma once
#include <audio-util/Util.h>
#include <filesystem>
#include <sndfile.h>

namespace AudioUtil
{
    /// @brief Decode a FLAC file and write the resulting WAV data to a VIO buffer.
    /// @param filepath Path to the input FLAC file.
    /// @param sf_vio Output VIO buffer to receive the decoded audio.
    void write_flac_to_vio(const std::filesystem::path &filepath, SF_VIO &sf_vio);
} // namespace AudioUtil
