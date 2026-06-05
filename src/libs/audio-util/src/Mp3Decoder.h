#ifndef MP3DECODER_H
#define MP3DECODER_H

#include <audio-util/SndfileVio.h>
#include <filesystem>

namespace AudioUtil
{
    bool write_mp3_to_vio(const std::filesystem::path &filepath, SF_VIO &sf_vio, std::string &msg);
} // namespace AudioUtil

#endif // MP3DECODER_H
