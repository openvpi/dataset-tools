#include <audio-util/Util.h>

#include <iostream>
#include <memory>
#include <soxr.h>
#include "FlacDecoder.h"
#include "Mp3Decoder.h"

#include <audio-util/PathCompat.h>

static std::vector<float> convertChannels(
    const std::vector<float> &output_buffer,
    size_t output_frames,
    size_t original_src_channels,
    size_t tar_channel)
{
    std::vector<float> processed_buffer(output_frames * tar_channel);

    if (tar_channel == 1 && original_src_channels > 1) {
        for (size_t i = 0; i < output_frames; ++i) {
            float mix = 0.0f;
            for (size_t ch = 0; ch < original_src_channels; ++ch) {
                size_t input_idx = i * original_src_channels + ch;
                if (input_idx < output_buffer.size()) {
                    mix += output_buffer[input_idx];
                }
            }
            processed_buffer[i] = mix / static_cast<float>(original_src_channels);
        }
    } else if (original_src_channels == 1 && tar_channel > 1) {
        for (size_t i = 0; i < output_frames; ++i) {
            float sample = (i < output_buffer.size()) ? output_buffer[i] : 0.0f;
            for (size_t ch = 0; ch < tar_channel; ++ch) {
                processed_buffer[i * tar_channel + ch] = sample;
            }
        }
    } else {
        const size_t min_channels = std::min<size_t>(original_src_channels, tar_channel);
        for (size_t i = 0; i < output_frames; ++i) {
            for (size_t ch = 0; ch < min_channels; ++ch) {
                size_t input_idx = i * original_src_channels + ch;
                size_t output_idx = i * tar_channel + ch;
                if (input_idx < output_buffer.size() && output_idx < processed_buffer.size()) {
                    processed_buffer[output_idx] = output_buffer[input_idx];
                }
            }
            for (size_t ch = min_channels; ch < tar_channel; ++ch) {
                size_t output_idx = i * tar_channel + ch;
                if (output_idx < processed_buffer.size()) {
                    processed_buffer[output_idx] = 0.0f;
                }
            }
        }
    }
    return processed_buffer;
}

namespace AudioUtil
{
    SF_VIO resample_to_vio(const std::filesystem::path &filepath, std::string &msg, const int tar_channel,
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
            return {};
        }

        SndfileHandle srcHandle;
        if (extension == ".wav") {
            srcHandle = openSndfile(filepath);
            if (!srcHandle) {
                msg = "Failed to open WAV file: " + std::string(sf_strerror(nullptr));
                return {};
            }
            sf_vio_in.info.frames = srcHandle.frames();
            sf_vio_in.info.format = srcHandle.format();
            sf_vio_in.info.channels = srcHandle.channels();
            sf_vio_in.info.samplerate = srcHandle.samplerate();
        } else {
            srcHandle = SndfileHandle(sf_vio_in.vio, &sf_vio_in.data, SFM_READ, sf_vio_in.info.format,
                                      sf_vio_in.info.channels, sf_vio_in.info.samplerate);
            if (!srcHandle) {
                msg = "Failed to open decoded audio: " + std::string(sf_strerror(nullptr));
                return {};
            }
        }

        const int original_src_channels = srcHandle.channels();
        const int original_src_samplerate = srcHandle.samplerate();

        int input_format = sf_vio_in.info.format & SF_FORMAT_SUBMASK;
        bool is_float_input = input_format == SF_FORMAT_FLOAT || input_format == SF_FORMAT_DOUBLE;

        SF_VIO sf_vio_out;
        sf_vio_out.info = sf_vio_in.info;
        sf_vio_out.info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        sf_vio_out.info.channels = tar_channel;
        sf_vio_out.info.samplerate = tar_samplerate;
        sf_vio_out.info.frames = 0;

        const double ratio = static_cast<double>(tar_samplerate) / original_src_samplerate;
        const sf_count_t estimated_frames = static_cast<sf_count_t>(srcHandle.frames() * ratio + 0.5);
        const size_t estimated_size = estimated_frames * tar_channel * sizeof(short);

        sf_vio_out.data.byteArray.reserve(estimated_size);

        auto dstHandle = SndfileHandle(sf_vio_out.vio, &sf_vio_out.data, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_16,
                                       tar_channel, tar_samplerate);
        if (!dstHandle) {
            msg = "Failed to open output VIO: " + std::string(sf_strerror(nullptr));
            return {};
        }

        soxr_error_t error = nullptr;
        soxr_io_spec_t io_spec;

        if (is_float_input) {
            io_spec = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);
        } else {
            int input_bits = 0;
            if (input_format == SF_FORMAT_PCM_16)
                input_bits = 16;
            else if (input_format == SF_FORMAT_PCM_24)
                input_bits = 24;
            else if (input_format == SF_FORMAT_PCM_32)
                input_bits = 32;

            if (input_bits == 16) {
                io_spec = soxr_io_spec(SOXR_INT16_I, SOXR_FLOAT32_I);
            } else {
                io_spec = soxr_io_spec(SOXR_INT32_I, SOXR_FLOAT32_I);
            }
        }

        const auto quality_spec = soxr_quality_spec(SOXR_HQ, 0);
        const auto runtime_spec = soxr_runtime_spec(1);

        soxr_t resampler = soxr_create(original_src_samplerate,
                                       tar_samplerate,
                                       original_src_channels,
                                       &error,
                                       &io_spec,
                                       &quality_spec,
                                       &runtime_spec
        );

        if (!resampler || error) {
            msg = "Failed to create SoX resampler: " + std::string(soxr_strerror(error ? error : soxr_error(0)));
            std::cerr << msg << std::endl;
            return {};
        }

        constexpr size_t BUFFER_FRAMES = 2048;
        const size_t input_buffer_size = BUFFER_FRAMES * original_src_channels;
        const size_t output_buffer_frames = static_cast<size_t>(BUFFER_FRAMES * ratio) + 1024;
        const size_t output_buffer_size = output_buffer_frames * original_src_channels;

        std::unique_ptr<float[]> float_input_buffer;
        std::unique_ptr<short[]> short_input_buffer;
        std::unique_ptr<int[]> int_input_buffer;
        std::vector<float> output_buffer(output_buffer_size);

        void *input_buffer_ptr = nullptr;

        if (is_float_input) {
            float_input_buffer = std::make_unique<float[]>(input_buffer_size);
            input_buffer_ptr = float_input_buffer.get();
        } else {
            if (input_format == SF_FORMAT_PCM_16) {
                short_input_buffer = std::make_unique<short[]>(input_buffer_size);
                input_buffer_ptr = short_input_buffer.get();
            } else {
                int_input_buffer = std::make_unique<int[]>(input_buffer_size);
                input_buffer_ptr = int_input_buffer.get();
            }
        }

        sf_count_t total_input_frames = 0;
        sf_count_t total_output_frames = 0;
        bool processing_error = false;

        while (!processing_error) {
            sf_count_t frames_read = 0;
            if (is_float_input) {
                frames_read = srcHandle.readf(static_cast<float *>(input_buffer_ptr), BUFFER_FRAMES);
            } else {
                if (input_format == SF_FORMAT_PCM_16) {
                    frames_read = srcHandle.readf(static_cast<short *>(input_buffer_ptr), BUFFER_FRAMES);
                } else {
                    sf_count_t items_read =
                        srcHandle.read(static_cast<int *>(input_buffer_ptr),
                                       static_cast<sf_count_t>(BUFFER_FRAMES * original_src_channels));
                    frames_read = items_read / original_src_channels;
                }
            }

            if (frames_read <= 0) {
                break;
            }

            total_input_frames += frames_read;

            const size_t input_total_samples = static_cast<size_t>(frames_read) * original_src_channels;
            size_t input_samples_processed = 0;

            while (input_samples_processed < input_total_samples && !processing_error) {
                size_t input_samples_remaining = input_total_samples - input_samples_processed;
                size_t max_input_frames_this_call = output_buffer_frames;
                size_t input_frames_this_call =
                    std::min(input_samples_remaining / original_src_channels, max_input_frames_this_call);

                void *current_input_ptr = static_cast<char *>(input_buffer_ptr) +
                    input_samples_processed *
                        (is_float_input                         ? sizeof(float)
                             : input_format == SF_FORMAT_PCM_16 ? sizeof(short)
                                                                : sizeof(int));

                size_t input_frames_done = 0;
                size_t output_frames_done = 0;

                error = soxr_process(resampler, current_input_ptr,
                                     input_frames_this_call,
                                     &input_frames_done, output_buffer.data(),
                                     output_buffer_frames,
                                     &output_frames_done);

                if (error) {
                    std::cerr << "Error during resampling: " << soxr_strerror(error) << std::endl;
                    processing_error = true;
                    break;
                }

                input_samples_processed += input_frames_done * original_src_channels;

                if (output_frames_done > 0) {
                    const size_t output_frames = output_frames_done;

                    auto processed_buffer = convertChannels(output_buffer, output_frames, original_src_channels, tar_channel);

                    const sf_count_t written = dstHandle.writef(processed_buffer.data(), output_frames);

                    if (written != static_cast<sf_count_t>(output_frames)) {
                        std::cerr << "Error writing to output VIO" << std::endl;
                        processing_error = true;
                        break;
                    }

                    total_output_frames += written;
                }
            }
        }

        if (!processing_error) {
            size_t output_frames_done = 0;
            do {
                error = soxr_process(resampler,
                                     nullptr,
                                     0,
                                     nullptr, output_buffer.data(),
                                     output_buffer_frames,
                                     &output_frames_done);

                if (error) {
                    std::cerr << "Error during resampling flush: " << soxr_strerror(error) << std::endl;
                    processing_error = true;
                    break;
                }

                if (output_frames_done > 0) {
                    const size_t output_frames = output_frames_done;

                    auto processed_buffer = convertChannels(output_buffer, output_frames, original_src_channels, tar_channel);

                    const sf_count_t written = dstHandle.writef(processed_buffer.data(), output_frames);

                    if (written != static_cast<sf_count_t>(output_frames)) {
                        std::cerr << "Error writing to output VIO during flush" << std::endl;
                        processing_error = true;
                        break;
                    }

                    total_output_frames += written;
                }
            }
            while (output_frames_done > 0 && !processing_error);
        }

        if (resampler) {
            soxr_delete(resampler);
            resampler = nullptr;
        }

        if (processing_error) {
            msg = "Processing error occurred during resampling";
            return {};
        }

        sf_vio_out.info.frames = total_output_frames;

        sf_vio_out.data.seek = 0;

        const double input_duration = static_cast<double>(total_input_frames) / original_src_samplerate;
        const double output_duration = static_cast<double>(total_output_frames) / tar_samplerate;

        if (std::abs(input_duration - output_duration) > 0.01) {
            std::cerr << "Resample Warning." << std::endl;
            std::cerr << "Input duration: " << input_duration << "s (" << total_input_frames << " frames)" << std::endl;
            std::cerr << "Output duration: " << output_duration << "s (" << total_output_frames << " frames)"
                      << std::endl;
            std::cerr << "Duration difference: " << (output_duration - input_duration) << "s" << std::endl;
        }

        return sf_vio_out;
    }

    bool write_vio_to_wav(SF_VIO &sf_vio_in, const std::filesystem::path &filepath, int tar_channel) {
        const auto [frames, samplerate, channels, format, sections, seekable] = sf_vio_in.info;

        if (tar_channel == -1)
            tar_channel = channels;

        sf_vio_in.data.seek = 0;
        SndfileHandle readBuf(sf_vio_in.vio, &sf_vio_in.data, SFM_READ, format, channels, samplerate);
        if (!readBuf) {
            std::cerr << "Failed to open input VIO for reading: " << sf_strerror(nullptr) << std::endl;
            return false;
        }

        auto outBuf = openSndfile(filepath, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_FLOAT, tar_channel, samplerate);
        if (!outBuf) {
            std::cerr << "Failed to open output WAV file: " << sf_strerror(nullptr) << std::endl;
            return false;
        }

        constexpr size_t BUFFER_FRAMES = 2048;
        std::vector<float> buffer(BUFFER_FRAMES * channels);
        sf_count_t readFrames;

        while ((readFrames = readBuf.readf(buffer.data(), BUFFER_FRAMES)) > 0) {
            if (tar_channel != channels) {
                std::vector<float> converted_buffer(static_cast<size_t>(readFrames) * tar_channel);

                if (tar_channel == 1 && channels > 1) {
                    for (sf_count_t i = 0; i < readFrames; ++i) {
                        float mix = 0.0f;
                        for (int ch = 0; ch < channels; ++ch) {
                            mix += buffer[i * channels + ch];
                        }
                        converted_buffer[i] = mix / static_cast<float>(channels);
                    }
                } else if (channels == 1 && tar_channel > 1) {
                    for (sf_count_t i = 0; i < readFrames; ++i) {
                        const float sample = buffer[i];
                        for (int ch = 0; ch < tar_channel; ++ch) {
                            converted_buffer[i * tar_channel + ch] = sample;
                        }
                    }
                } else {
                    const int min_channels = std::min(channels, tar_channel);
                    for (sf_count_t i = 0; i < readFrames; ++i) {
                        for (int ch = 0; ch < min_channels; ++ch) {
                            converted_buffer[i * tar_channel + ch] = buffer[i * channels + ch];
                        }
                        for (int ch = min_channels; ch < tar_channel; ++ch) {
                            converted_buffer[i * tar_channel + ch] = 0.0f;
                        }
                    }
                }
                outBuf.writef(converted_buffer.data(), readFrames);
            } else {
                outBuf.writef(buffer.data(), readFrames);
            }
        }

        return true;
    }
} // namespace AudioUtil
