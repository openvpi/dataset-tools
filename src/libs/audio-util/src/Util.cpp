#include <audio-util/Util.h>

#include <iostream>
#include <soxr.h>
#include "FlacDecoder.h"
#include "Mp3Decoder.h"

namespace AudioUtil
{
    SF_VIO resample_to_vio(const std::filesystem::path &filepath, std::string &msg, const int tar_channels,
                           const int tar_samplerate) {
        SF_VIO sf_vio_in;
        const std::string extension = filepath.extension().string();
        if (extension == ".wav") {
        } else if (extension == ".mp3") {
            write_mp3_to_vio(filepath, sf_vio_in);
            sf_vio_in.data.seek = 0;
        } else if (extension == ".flac") {
            write_flac_to_vio(filepath, sf_vio_in);
            sf_vio_in.data.seek = 0;
        } else {
            msg = "Unsupported file format: " + filepath.string();
        }

        SndfileHandle srcHandle;
        if (extension == ".wav") {
            srcHandle = SndfileHandle(filepath.string());
            sf_vio_in.info.frames = srcHandle.frames();
            sf_vio_in.info.format = srcHandle.format();
        } else {
            srcHandle = SndfileHandle(sf_vio_in.vio, &sf_vio_in.data, SFM_READ, sf_vio_in.info.format,
                                      sf_vio_in.info.channels, sf_vio_in.info.samplerate);
        }

        SF_VIO sf_vio;
        sf_vio.info = sf_vio_in.info;
        sf_vio.info.channels = tar_channels;
        sf_vio.info.samplerate = tar_samplerate;
        if (!srcHandle) {
            std::cout << "Failed to open WAV file:" << sf_strerror(nullptr) << std::endl;
            return {};
        }

        const sf_count_t estimated_frames = (srcHandle.frames() * tar_samplerate) / srcHandle.samplerate();
        const size_t estimated_size = estimated_frames * srcHandle.channels() * sizeof(float);

        sf_vio.data.byteArray.reserve(estimated_size);

        auto dstHandle =
            SndfileHandle(sf_vio.vio, &sf_vio.data, SFM_WRITE, sf_vio_in.info.format, tar_channels, tar_samplerate);
        if (!dstHandle) {
            std::cout << "Failed to open output file:" << sf_strerror(nullptr) << std::endl;
            return {};
        }

        std::cout << "Start resample." << std::endl;
        // Create a SoX resampler instance
        soxr_error_t error;
        const auto resampler = soxr_create(srcHandle.samplerate(), tar_samplerate, srcHandle.channels(), &error,
                                           nullptr, nullptr, nullptr);
        if (!resampler) {
            std::cout << "Failed to create SoX resampler: " << soxr_strerror(error) << std::endl;
            return {};
        }

        // Define buffers for resampling
        std::vector<float> inputBuf(srcHandle.samplerate() * srcHandle.channels(), 0);
        std::vector<float> outputBuf(tar_samplerate * srcHandle.channels(), 0);

        size_t bytesRead;

        while ((bytesRead = srcHandle.read(inputBuf.data(), static_cast<sf_count_t>(inputBuf.size()))) > 0) {
            const size_t inputSamples = bytesRead / srcHandle.channels();
            const size_t outputSamples = outputBuf.size() / srcHandle.channels();

            // Perform the resampling using SoX
            size_t inputDone = 0, outputDone = 0;
            const soxr_error_t err = soxr_process(resampler, inputBuf.data(), inputSamples, &inputDone,
                                                  outputBuf.data(), outputSamples, &outputDone);

            if (err != nullptr) {
                std::cout << "Error during resampling: " << soxr_strerror(err) << std::endl;
                break;
            }

            std::vector<float> processedBuf(outputDone * tar_channels);
            const auto src_channels = srcHandle.channels();
            for (size_t i = 0; i < outputDone; ++i) {
                for (size_t tar_ch = 0; tar_ch < tar_channels; ++tar_ch) {
                    if (tar_channels > src_channels) {
                        const size_t src_ch = tar_ch % src_channels;
                        processedBuf[i * tar_channels + tar_ch] = outputBuf[i * src_channels + src_ch];
                    } else {
                        const size_t src_ch = tar_ch;
                        processedBuf[i * tar_channels + tar_ch] = outputBuf[i * src_channels + src_ch];
                    }
                }
            }

            const size_t bytesWritten =
                dstHandle.write(processedBuf.data(), static_cast<sf_count_t>(outputDone * tar_channels));
            if (bytesWritten != outputDone * tar_channels) {
                std::cout << "Error writing to output file" << std::endl;
                break;
            }
        }

        // Clean up
        soxr_delete(resampler);
        std::cout << "Resample success." << std::endl;
        return sf_vio;
    }

    bool write_vio_to_wav(SF_VIO &sf_vio_in, const std::filesystem::path &filepath, int tar_channel) {
        const auto [frames, samplerate, channels, format, sections, seekable] = sf_vio_in.info;

        if (tar_channel == -1)
            tar_channel = channels;

        SndfileHandle readBuf(sf_vio_in.vio, &sf_vio_in.data, SFM_READ, format, channels, samplerate);

        SndfileHandle outBuf(filepath.string(), SFM_WRITE, format, tar_channel, samplerate);

        std::vector<float> buffer(1024 * channels, 0);
        int readFrames;

        while ((readFrames = static_cast<int>(
                    readBuf.readf(buffer.data(), static_cast<sf_count_t>(buffer.size()) / channels))) > 0) {
            if (tar_channel != channels) {
                std::vector<float> convertedBuffer(1024 * tar_channel, 0);
                for (int i = 0; i < readFrames; ++i) {
                    for (int ch = 0; ch < tar_channel; ++ch) {
                        if (ch < channels) {
                            convertedBuffer[i * tar_channel + ch] = buffer[i * channels + ch];
                        } else {
                            convertedBuffer[i * tar_channel + ch] = 0;
                        }
                    }
                }
                outBuf.writef(convertedBuffer.data(), readFrames);
            } else {
                outBuf.writef(buffer.data(), readFrames);
            }
        }
        return true;
    }
} // namespace AudioUtil
