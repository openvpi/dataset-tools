#include <dsfw/audio/SwresampleResampler.h>

extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>
}

#include <algorithm>
#include <cstring>
#include <vector>

namespace dsfw::audio {

    namespace {
        /// @brief Convert FFmpeg error code to human-readable string.
        std::string ffmpegError(int err) {
            char buf[AV_ERROR_MAX_STRING_SIZE];
            return av_make_error_string(buf, AV_ERROR_MAX_STRING_SIZE, err);
        }
        AVSampleFormat toAVSampleFormat(SampleFormat fmt) {
            switch (fmt) {
            case SampleFormat::Float32: return AV_SAMPLE_FMT_FLT;
            case SampleFormat::Int16:   return AV_SAMPLE_FMT_S16;
            case SampleFormat::Int32:   return AV_SAMPLE_FMT_S32;
            default: return AV_SAMPLE_FMT_FLT;
            }
        }

        SampleFormat fromAVSampleFormat(AVSampleFormat fmt) {
            switch (fmt) {
            case AV_SAMPLE_FMT_FLT:
            case AV_SAMPLE_FMT_FLTP: return SampleFormat::Float32;
            case AV_SAMPLE_FMT_S16:
            case AV_SAMPLE_FMT_S16P: return SampleFormat::Int16;
            case AV_SAMPLE_FMT_S32:
            case AV_SAMPLE_FMT_S32P: return SampleFormat::Int32;
            default: return SampleFormat::Float32;
            }
        }

        int qualityToAvFlags(ResampleQuality quality) {
            switch (quality) {
            case ResampleQuality::Draft:  return 0;
            case ResampleQuality::Medium: return SWR_FLAG_RESAMPLE;
            case ResampleQuality::High:   return 0; // Use dither
            case ResampleQuality::Ultra:  return SWR_FLAG_RESAMPLE;
            default: return 0;
            }
        }
    } // anonymous namespace

    struct SwresampleResampler::Impl {
        SwrContext *swrCtx = nullptr;
        int64_t inSampleRate = 0;
        int inChannels = 0;
        AVSampleFormat inFormat = AV_SAMPLE_FMT_FLT;
        int64_t outSampleRate = 0;
        int outChannels = 0;
        AVSampleFormat outFormat = AV_SAMPLE_FMT_FLT;

        void cleanup() {
            if (swrCtx) {
                swr_free(&swrCtx);
                swrCtx = nullptr;
            }
        }

        dsfw::Result<void> init(const AudioBuffer &input, int inputSampleRate, const ResampleConfig &config) {
            auto srcFmt = toAVSampleFormat(input.format());
            auto dstFmt = (config.targetFormat != SampleFormat::Unknown)
                              ? toAVSampleFormat(config.targetFormat)
                              : srcFmt;
            auto srcRate = inputSampleRate;
            auto dstRate = (config.targetSampleRate > 0) ? config.targetSampleRate : srcRate;
            auto srcCh = input.channelCount();
            auto dstCh = (config.targetChannelCount > 0) ? config.targetChannelCount : srcCh;

            // If no change needed, skip swresample
            bool same = (srcFmt == dstFmt) && (srcRate == dstRate) && (srcCh == dstCh);
            if (same) {
                inSampleRate = srcRate;
                inChannels = srcCh;
                inFormat = srcFmt;
                outSampleRate = dstRate;
                outChannels = dstCh;
                outFormat = dstFmt;
                return dsfw::Result<void>::Ok();
            }

            cleanup();

            AVChannelLayout inChLayout{};
            av_channel_layout_default(&inChLayout, srcCh);
            AVChannelLayout outChLayout{};
            av_channel_layout_default(&outChLayout, dstCh);

            int ret = swr_alloc_set_opts2(&swrCtx, &outChLayout, dstFmt, dstRate, &inChLayout, srcFmt, srcRate, 0,
                                          nullptr);
            av_channel_layout_uninit(&inChLayout);
            av_channel_layout_uninit(&outChLayout);

            if (ret < 0) {
                return dsfw::Result<void>::Error("Failed to set resampler options: " + ffmpegError(ret));
            }

            // Apply quality flags
            int flags = qualityToAvFlags(config.quality);
            if (flags) {
                av_opt_set_int(swrCtx, "flags", flags, 0);
            }

            ret = swr_init(swrCtx);
            if (ret < 0) {
                cleanup();
                return dsfw::Result<void>::Error("Failed to initialize resampler: " + ffmpegError(ret));
            }

            inSampleRate = srcRate;
            inChannels = srcCh;
            inFormat = srcFmt;
            outSampleRate = dstRate;
            outChannels = dstCh;
            outFormat = dstFmt;

            return dsfw::Result<void>::Ok();
        }
    };

    SwresampleResampler::SwresampleResampler() : d(std::make_unique<Impl>()) {
    }

    SwresampleResampler::~SwresampleResampler() {
        d->cleanup();
    }

    SwresampleResampler::SwresampleResampler(SwresampleResampler &&) noexcept = default;
    SwresampleResampler &SwresampleResampler::operator=(SwresampleResampler &&) noexcept = default;

    dsfw::Result<AudioBuffer> SwresampleResampler::convert(const AudioBuffer &input, int inputSampleRate,
                                                              const ResampleConfig &config) {
        if (input.empty()) {
            return AudioBuffer::create(0, config.targetChannelCount > 0 ? config.targetChannelCount
                                                                        : input.channelCount(),
                                       config.targetFormat != SampleFormat::Unknown ? config.targetFormat
                                                                                    : input.format());
        }

        auto initResult = d->init(input, inputSampleRate, config);
        if (!initResult) {
            return dsfw::Result<AudioBuffer>::Error(initResult.error());
        }

        // If no resampling needed (same format), return copy
        if (!d->swrCtx) {
            return input.clone();
        }

        // Perform resampling
        int srcBps = bytesPerSample(input.format());
        int dstBps = bytesPerSample(fromAVSampleFormat(d->outFormat));
        int outCh = d->outChannels;

        // Estimate output frames
        int64_t estimatedOutFrames = swr_get_out_samples(d->swrCtx, static_cast<int>(input.frameCount()));
        if (estimatedOutFrames <= 0) {
            estimatedOutFrames = input.frameCount();
        }

        std::vector<uint8_t> outData;
        outData.reserve(static_cast<size_t>(estimatedOutFrames * outCh * dstBps));

        const auto *inData = reinterpret_cast<const uint8_t *>(input.rawData());
        int inFrames = static_cast<int>(input.frameCount());

        // Convert in chunks to avoid large intermediate buffers
        constexpr int kChunkFrames = 4096;
        int inOffset = 0;

        while (inOffset < inFrames) {
            int chunkFrames = std::min(kChunkFrames, inFrames - inOffset);
            const uint8_t *inPtr = inData + inOffset * input.channelCount() * srcBps;

            int outSamples = swr_get_out_samples(d->swrCtx, chunkFrames);
            std::vector<uint8_t> chunkBuf(outSamples * outCh * dstBps);
            auto *outPtr = chunkBuf.data();

            int converted = swr_convert(d->swrCtx, &outPtr, outSamples, &inPtr, chunkFrames);
            if (converted < 0) {
                return dsfw::Result<AudioBuffer>::Error("swr_convert failed: " + ffmpegError(converted));
            }
            if (converted > 0) {
                outData.insert(outData.end(), chunkBuf.begin(), chunkBuf.begin() + converted * outCh * dstBps);
            }

            inOffset += chunkFrames;
        }

        // Flush resampler
        {
            int outSamples = swr_get_out_samples(d->swrCtx, 0);
            if (outSamples > 0) {
                std::vector<uint8_t> flushBuf(outSamples * outCh * dstBps);
                auto *outPtr = flushBuf.data();
                int converted = swr_convert(d->swrCtx, &outPtr, outSamples, nullptr, 0);
                if (converted < 0) {
                    return dsfw::Result<AudioBuffer>::Error("swr_convert flush failed: " + ffmpegError(converted));
                }
                if (converted > 0) {
                    outData.insert(outData.end(), flushBuf.begin(), flushBuf.begin() + converted * outCh * dstBps);
                }
            }
        }

        auto outFormat = fromAVSampleFormat(d->outFormat);
        int64_t outFrames = static_cast<int64_t>(outData.size() / (outCh * dstBps));
        return AudioBuffer::fromVector(std::move(outData), outFrames, outCh, outFormat);
    }

} // namespace dsfw::audio