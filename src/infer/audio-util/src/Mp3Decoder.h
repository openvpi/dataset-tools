/// @file Mp3Decoder.h
/// @brief MP3 to WAV decoder writing to in-memory VIO.

#ifndef MP3DECODER_H
#define MP3DECODER_H

#include <audio-util/SndfileVio.h>
#include <filesystem>

namespace AudioUtil
{
    /// @brief Decode an MP3 file and write the resulting WAV data to a VIO buffer.
    /// @param filepath Path to the input MP3 file.
    /// @param sf_vio Output VIO buffer to receive the decoded audio.
    void write_mp3_to_vio(const std::filesystem::path &filepath, SF_VIO &sf_vio);
} // namespace AudioUtil

#endif // MP3DECODER_H
