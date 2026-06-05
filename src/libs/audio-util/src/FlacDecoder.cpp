#include "FlacDecoder.h"

#include <iostream>

namespace AudioUtil
{
    bool write_flac_to_vio(const std::filesystem::path &filepath, SF_VIO &sf_vio, std::string &msg) {
        SndfileHandle infile(filepath.string(), SFM_READ);
        if (!infile) {
            msg = "Unable to open input file for reading: " + filepath.string() + " Error: " + infile.strError();
            return false;
        }

        const auto format = infile.format();
        sf_vio.info.samplerate = infile.samplerate();
        sf_vio.info.frames = infile.frames();
        sf_vio.info.channels = infile.channels();

        int bit_depth;
        if (format & SF_FORMAT_PCM_16) {
            bit_depth = 16;
        } else if (format & SF_FORMAT_PCM_24) {
            bit_depth = 24;
        } else if (format & SF_FORMAT_PCM_32) {
            bit_depth = 32;
        } else {
            msg = "Unsupported FLAC bit depth";
            return false;
        }

        int output_format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        if (bit_depth == 24) {
            output_format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
        } else if (bit_depth == 32) {
            output_format = SF_FORMAT_WAV | SF_FORMAT_PCM_32;
        }

        sf_vio.info.format = output_format;

        SndfileHandle outfile(sf_vio.vio, &sf_vio.data, SFM_WRITE, output_format, sf_vio.info.channels,
                              sf_vio.info.samplerate);
        if (!outfile) {
            msg = "Unable to open output VIO for writing. Error message: " + std::string(outfile.strError());
            return false;
        }

        const size_t estimated_size = sf_vio.info.frames * sf_vio.info.channels * sizeof(float);
        sf_vio.data.byteArray.reserve(estimated_size);

        constexpr int buffer_size = 1024;
        std::vector<float> buffer(buffer_size * sf_vio.info.channels);

        sf_count_t frames_read;
        while ((frames_read = infile.read(&buffer[0], buffer_size)) > 0) {
            if (outfile.write(&buffer[0], frames_read) != frames_read) {
                msg = "Error writing data to VIO for: " + filepath.string();
                return false;
            }
        }

        return true;
    }

} // namespace AudioUtil
