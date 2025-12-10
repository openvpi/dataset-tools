#include <audio-util/Util.h>

#include <iostream>
#include <soxr.h>
#include "FlacDecoder.h"
#include "Mp3Decoder.h"

namespace AudioUtil
{
    SF_VIO resample_to_vio(const std::filesystem::path &filepath, std::string &msg, const int tar_channel,
                           const int tar_samplerate) {
        SF_VIO sf_vio_in;
        const std::string extension = filepath.extension().string();

        // 处理不同格式的音频文件
        if (extension == ".wav") {
            // WAV文件直接使用SndfileHandle打开
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
            srcHandle = SndfileHandle(filepath.string());
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

        SF_VIO sf_vio_out;
        sf_vio_out.info = sf_vio_in.info;
        sf_vio_out.info.channels = tar_channel;
        sf_vio_out.info.samplerate = tar_samplerate;
        sf_vio_out.info.frames = 0; // 重置帧数，重新计算

        // 计算估计的输出帧数
        const double ratio = static_cast<double>(tar_samplerate) / srcHandle.samplerate();
        const sf_count_t estimated_frames = static_cast<sf_count_t>(srcHandle.frames() * ratio + 0.5);
        const size_t estimated_size = estimated_frames * tar_channel * sizeof(float);

        sf_vio_out.data.byteArray.reserve(estimated_size);

        // 创建输出VIO的SndfileHandle
        auto dstHandle = SndfileHandle(sf_vio_out.vio, &sf_vio_out.data, SFM_WRITE, sf_vio_in.info.format, tar_channel,
                                       tar_samplerate);
        if (!dstHandle) {
            msg = "Failed to open output VIO: " + std::string(sf_strerror(nullptr));
            return {};
        }

        std::cout << "Start resample from " << srcHandle.samplerate() << "Hz to " << tar_samplerate << "Hz"
                  << std::endl;
        std::cout << "Channels: " << srcHandle.channels() << " -> " << tar_channel << std::endl;

        // 创建 SoX 重采样器
        soxr_error_t error;
        const auto io_spec = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I);
        const auto quality_spec = soxr_quality_spec(SOXR_HQ, 0);
        const auto runtime_spec = soxr_runtime_spec(1); // 单线程

        const auto resampler = soxr_create(static_cast<double>(srcHandle.samplerate()), // 输入采样率
                                           static_cast<double>(tar_samplerate), // 输出采样率
                                           srcHandle.channels(), // 通道数
                                           &error, // 错误信息
                                           &io_spec, // I/O 规格
                                           &quality_spec, // 质量规格
                                           &runtime_spec // 运行时规格
        );

        if (!resampler) {
            msg = "Failed to create SoX resampler: " + std::string(soxr_strerror(error));
            std::cout << msg << std::endl;
            return {};
        }

        // 使用适当的缓冲区大小
        constexpr size_t BUFFER_FRAMES = 4096;
        const size_t input_buffer_size = BUFFER_FRAMES * srcHandle.channels();
        const size_t output_buffer_size = static_cast<size_t>(BUFFER_FRAMES * ratio * srcHandle.channels()) + 1024;

        std::vector<float> input_buffer(input_buffer_size);
        std::vector<float> output_buffer(output_buffer_size);

        sf_count_t total_input_frames = 0;
        sf_count_t total_output_frames = 0;

        while (true) {
            // 从源文件读取数据
            const sf_count_t frames_read = srcHandle.readf(input_buffer.data(), BUFFER_FRAMES);

            if (frames_read <= 0) {
                break;
            }

            total_input_frames += frames_read;

            // 计算这次读取的实际输入样本数（样本数 = 帧数 × 通道数）
            const size_t input_samples = frames_read * srcHandle.channels();

            // 处理重采样
            size_t input_done = 0;

            while (input_done < input_samples) {
                size_t output_done = 0;
                const size_t remaining_input = input_samples - input_done;
                const size_t max_output_samples = output_buffer.size();

                error = soxr_process(resampler,
                                     input_buffer.data() + input_done, // 输入指针
                                     remaining_input, // 输入样本数
                                     &input_done, // 实际处理的输入样本数
                                     output_buffer.data(), // 输出缓冲区
                                     max_output_samples, // 输出缓冲区容量
                                     &output_done // 实际输出的样本数
                );

                if (error) {
                    std::cout << "Error during resampling: " << soxr_strerror(error) << std::endl;
                    break;
                }

                // 处理输出数据
                if (output_done > 0) {
                    const size_t output_frames = output_done / srcHandle.channels();

                    // 通道转换
                    std::vector<float> processed_buffer(output_frames * tar_channel);

                    const auto src_channels = srcHandle.channels();

                    if (tar_channel == 1 && src_channels > 1) {
                        // 混音到单声道
                        for (size_t i = 0; i < output_frames; ++i) {
                            float mix = 0.0f;
                            for (size_t ch = 0; ch < src_channels; ++ch) {
                                mix += output_buffer[i * src_channels + ch];
                            }
                            processed_buffer[i] = mix / static_cast<float>(src_channels);
                        }
                    } else if (src_channels == 1 && tar_channel > 1) {
                        // 单声道复制到多声道
                        for (size_t i = 0; i < output_frames; ++i) {
                            const float sample = output_buffer[i];
                            for (size_t ch = 0; ch < tar_channel; ++ch) {
                                processed_buffer[i * tar_channel + ch] = sample;
                            }
                        }
                    } else {
                        // 通道数相同或不同时的处理
                        const size_t min_channels = std::min<size_t>(src_channels, tar_channel);
                        for (size_t i = 0; i < output_frames; ++i) {
                            for (size_t ch = 0; ch < min_channels; ++ch) {
                                processed_buffer[i * tar_channel + ch] = output_buffer[i * src_channels + ch];
                            }
                            for (size_t ch = min_channels; ch < tar_channel; ++ch) {
                                processed_buffer[i * tar_channel + ch] = 0.0f;
                            }
                        }
                    }

                    // 写入目标VIO
                    const sf_count_t written = dstHandle.writef(processed_buffer.data(), output_frames);

                    if (written != output_frames) {
                        std::cout << "Error writing to output VIO" << std::endl;
                        break;
                    }

                    total_output_frames += written;
                }
            }

            if (error) {
                break;
            }
        }

        // 刷新重采样器以获取剩余数据
        if (!error) {
            size_t output_done = 0;
            do {
                const size_t max_output_samples = output_buffer.size();

                error = soxr_process(resampler,
                                     nullptr, // 输入结束
                                     0, // 没有更多输入
                                     nullptr,
                                     output_buffer.data(), // 输出缓冲区
                                     max_output_samples, // 输出缓冲区容量
                                     &output_done // 实际输出的样本数
                );

                if (output_done > 0) {
                    const size_t output_frames = output_done / srcHandle.channels();

                    // 通道转换
                    std::vector<float> processed_buffer(output_frames * tar_channel);

                    const auto src_channels = srcHandle.channels();

                    if (tar_channel == 1 && src_channels > 1) {
                        for (size_t i = 0; i < output_frames; ++i) {
                            float mix = 0.0f;
                            for (size_t ch = 0; ch < src_channels; ++ch) {
                                mix += output_buffer[i * src_channels + ch];
                            }
                            processed_buffer[i] = mix / static_cast<float>(src_channels);
                        }
                    } else if (src_channels == 1 && tar_channel > 1) {
                        for (size_t i = 0; i < output_frames; ++i) {
                            const float sample = output_buffer[i];
                            for (size_t ch = 0; ch < tar_channel; ++ch) {
                                processed_buffer[i * tar_channel + ch] = sample;
                            }
                        }
                    } else {
                        const size_t min_channels = std::min<size_t>(src_channels, tar_channel);
                        for (size_t i = 0; i < output_frames; ++i) {
                            for (size_t ch = 0; ch < min_channels; ++ch) {
                                processed_buffer[i * tar_channel + ch] = output_buffer[i * src_channels + ch];
                            }
                            for (size_t ch = min_channels; ch < tar_channel; ++ch) {
                                processed_buffer[i * tar_channel + ch] = 0.0f;
                            }
                        }
                    }

                    const sf_count_t written = dstHandle.writef(processed_buffer.data(), output_frames);

                    if (written != output_frames) {
                        std::cout << "Error writing to output VIO" << std::endl;
                        break;
                    }

                    total_output_frames += written;
                }
            }
            while (output_done > 0 && !error);
        }

        // 清理重采样器
        soxr_delete(resampler);

        // 更新输出VIO的帧数信息
        sf_vio_out.info.frames = total_output_frames;

        // 计算并显示时长信息
        const double input_duration = static_cast<double>(total_input_frames) / srcHandle.samplerate();
        const double output_duration = static_cast<double>(total_output_frames) / tar_samplerate;

        std::cout << "Resample success." << std::endl;
        std::cout << "Input duration: " << input_duration << "s (" << total_input_frames << " frames)" << std::endl;
        std::cout << "Output duration: " << output_duration << "s (" << total_output_frames << " frames)" << std::endl;
        std::cout << "Duration difference: " << (output_duration - input_duration) << "s" << std::endl;

        return sf_vio_out;
    }

    bool write_vio_to_wav(SF_VIO &sf_vio_in, const std::filesystem::path &filepath, int tar_channel) {
        const auto [frames, samplerate, channels, format, sections, seekable] = sf_vio_in.info;

        if (tar_channel == -1)
            tar_channel = channels;

        // 从VIO读取
        SndfileHandle readBuf(sf_vio_in.vio, &sf_vio_in.data, SFM_READ, format, channels, samplerate);
        if (!readBuf) {
            std::cerr << "Failed to open input VIO for reading: " << sf_strerror(nullptr) << std::endl;
            return false;
        }

        // 创建输出WAV文件
        SndfileHandle outBuf(filepath.string(), SFM_WRITE, format, tar_channel, samplerate);
        if (!outBuf) {
            std::cerr << "Failed to open output WAV file: " << sf_strerror(nullptr) << std::endl;
            return false;
        }

        constexpr size_t BUFFER_FRAMES = 4096;
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

        std::cout << "Successfully wrote VIO to WAV file: " << filepath << std::endl;
        return true;
    }
} // namespace AudioUtil
