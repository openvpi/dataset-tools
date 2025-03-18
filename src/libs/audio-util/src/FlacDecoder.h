#ifndef FLACDECODER_H
#define FLACDECODER_H

#include <audio-util/Util.h>
#include <filesystem>
#include <sndfile.h>

namespace AudioUtil
{
    void write_flac_to_vio(const std::filesystem::path &filepath, SF_VIO &sf_vio);
} // namespace AudioUtil

#endif // FLACDECODER_H
