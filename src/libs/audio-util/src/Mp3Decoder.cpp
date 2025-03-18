#include "Mp3Decoder.h"

#include <algorithm>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <mpg123.h>
#include <vector>

namespace AudioUtil
{
    void write_mp3_to_vio(const std::filesystem::path &filepath, SF_VIO &sf_vio) {
        if (mpg123_init() != MPG123_OK) {
            std::cerr << "Failed to initialize mpg123" << std::endl;
            return;
        }

        mpg123_handle *mh = mpg123_new(nullptr, nullptr);
        if (mh == nullptr) {
            std::cerr << "Failed to create mpg123 handle" << std::endl;
            mpg123_exit();
            return;
        }

        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open MP3 file: " << filepath << std::endl;
            mpg123_delete(mh);
            mpg123_exit();
            return;
        }

        std::vector<unsigned char> mp3_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();

        if (mpg123_open_feed(mh) != MPG123_OK) {
            std::cerr << "Failed to open feed" << std::endl;
            mpg123_delete(mh);
            mpg123_exit();
            return;
        }

        size_t pos = 0;
        while (pos < mp3_data.size()) {
            constexpr size_t buffer_size = 8192;
            unsigned char buffer[buffer_size];
            const size_t feed_size = std::min(buffer_size, mp3_data.size() - pos);
            std::copy_n(mp3_data.begin() + static_cast<long long>(pos), feed_size, buffer);

            const int feed_result = mpg123_feed(mh, buffer, feed_size);
            if (feed_result != MPG123_OK) {
                std::cerr << "Failed to feed data to mpg123: " << mpg123_strerror(mh) << std::endl;
                break;
            }
            pos += feed_size;
        }

        long rate;
        int channels, encoding;
        if (mpg123_getformat(mh, &rate, &channels, &encoding) != MPG123_OK) {
            std::cerr << "Failed to get format from mpg123: " << mpg123_strerror(mh) << std::endl;
            mpg123_delete(mh);
            mpg123_exit();
            return;
        }

        const off_t total_frames = mpg123_length(mh);
        if (total_frames < 0) {
            std::cerr << "Failed to get frame count: " << mpg123_strerror(mh) << std::endl;
        } else {
            std::cout << "Total Frames: " << total_frames << std::endl;
        }

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

        std::cout << "Rate: " << rate << ", Channels: " << channels << ", Encoding: " << encoding << std::endl;

        const size_t estimated_size = total_frames * channels * sizeof(float);
        sf_vio.data.byteArray.reserve(estimated_size);

        SndfileHandle outBuf(sf_vio.vio, &sf_vio.data, SFM_WRITE, sf_vio.info.format, channels, rate);

        if (encoding & MPG123_ENC_FLOAT_32) {
            // 32-bit float buffer for multi-channel audio
            std::vector<float> pcm_buffer(8192 * channels, 0);
            size_t done;
            while (true) {
                const int err = mpg123_read(mh, pcm_buffer.data(), pcm_buffer.size() * sizeof(float), &done);
                if (err != MPG123_OK) {
                    if (done == 0) {
                        break;
                    }
                    // std::cerr << "Error while decoding MP3: " << mpg123_strerror(mh) << std::endl;
                    break;
                }

                // Assuming `done` contains bytes read, convert to number of samples
                const auto num_samples = static_cast<sf_count_t>(done / sizeof(float));
                const sf_count_t written = outBuf.writef(pcm_buffer.data(), num_samples);

                if (written > 0)
                    sf_vio.info.frames += written;
            }
        } else {
            // 16-bit integer buffer for multi-channel audio
            std::vector<short> pcm_buffer(8192 * channels, 0);
            size_t done;
            while (true) {
                const int err = mpg123_read(mh, pcm_buffer.data(), pcm_buffer.size() * sizeof(short), &done);
                if (err != MPG123_OK) {
                    if (done == 0) {
                        break;
                    }
                    // std::cerr << "Error while decoding MP3: " << mpg123_strerror(mh) << std::endl;
                    break;
                }

                // Assuming `done` contains bytes read, convert to number of samples
                const auto num_samples = static_cast<sf_count_t>(done / sizeof(short));
                const sf_count_t written = outBuf.write(pcm_buffer.data(), num_samples);

                if (written > 0)
                    sf_vio.info.frames += written;
            }
        }

        mpg123_delete(mh);
        mpg123_exit();
        std::cout << "Mp3 decode success." << std::endl;
    }
} // namespace AudioUtil
