#include "Mp3Decoder.h"

#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <mpg123.h>
#include <vector>

namespace AudioUtil
{
    bool write_mp3_to_vio(const std::filesystem::path &filepath, SF_VIO &sf_vio, std::string &msg) {
        if (mpg123_init() != MPG123_OK) {
            msg = "Failed to initialize mpg123";
            return false;
        }

        mpg123_handle *mh = mpg123_new(nullptr, nullptr);
        if (mh == nullptr) {
            msg = "Failed to create mpg123 handle";
            mpg123_exit();
            return false;
        }

        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            msg = "Failed to open MP3 file: " + filepath.string();
            mpg123_delete(mh);
            mpg123_exit();
            return false;
        }

        std::vector<unsigned char> mp3_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        if (mpg123_open_feed(mh) != MPG123_OK) {
            msg = "Failed to open feed";
            mpg123_delete(mh);
            mpg123_exit();
            return false;
        }

        size_t pos = 0;
        while (pos < mp3_data.size()) {
            constexpr size_t buffer_size = 8192;
            unsigned char buffer[buffer_size];
            const size_t feed_size = std::min(buffer_size, mp3_data.size() - pos);
            std::copy_n(mp3_data.begin() + static_cast<long long>(pos), feed_size, buffer);

            const int feed_result = mpg123_feed(mh, buffer, feed_size);
            if (feed_result != MPG123_OK) {
                msg = "Failed to feed data to mpg123: " + std::string(mpg123_strerror(mh));
                mpg123_delete(mh);
                mpg123_exit();
                return false;
            }
            pos += feed_size;
        }

        long rate;
        int channels, encoding;
        if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
            msg = "Failed to get format from mpg123: " + std::string(mpg123_strerror(mh));
            mpg123_delete(mh);
            mpg123_exit();
            return false;
        }

        const off_t total_frames = mpg123_length(mh);

        if (encoding & MPG123_ENC_FLOAT_32) {
            sf_vio.info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
        } else {
            sf_vio.info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        }

        sf_vio.info.samplerate = rate;
        sf_vio.info.channels = channels;
        sf_vio.info.frames = 0;
        sf_vio.info.sections = 1;
        sf_vio.info.seekable = 1;

        const size_t estimated_size = total_frames * channels * sizeof(float);
        sf_vio.data.byteArray.reserve(estimated_size);

        SndfileHandle outBuf(sf_vio.vio, &sf_vio.data, SFM_WRITE, sf_vio.info.format, channels, rate);

        if (encoding & MPG123_ENC_FLOAT_32) {
            std::vector<float> pcm_buffer(8192 * channels, 0);
            size_t done;
            while (true) {
                const int err = mpg123_read(mh, pcm_buffer.data(), pcm_buffer.size() * sizeof(float), &done);
                if (err != MPG123_OK) {
                    if (done == 0) {
                        break;
                    }
                    break;
                }

                const auto num_samples = static_cast<sf_count_t>(done / sizeof(float));
                const sf_count_t written = outBuf.writef(pcm_buffer.data(), num_samples);

                if (written > 0)
                    sf_vio.info.frames += written;
            }
        } else {
            std::vector<short> pcm_buffer(8192 * channels, 0);
            size_t done;
            while (true) {
                const int err = mpg123_read(mh, pcm_buffer.data(), pcm_buffer.size() * sizeof(short), &done);
                if (err != MPG123_OK) {
                    if (done == 0) {
                        break;
                    }
                    break;
                }

                const auto num_samples = static_cast<sf_count_t>(done / sizeof(short));
                const sf_count_t written = outBuf.write(pcm_buffer.data(), num_samples);

                if (written > 0)
                    sf_vio.info.frames += written;
            }
        }

        mpg123_delete(mh);
        mpg123_exit();
        return true;
    }
} // namespace AudioUtil
