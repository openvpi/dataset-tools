#include <dsfw/audio/FfmpegAudioDecoder.h>
#include "FfmpegUtils.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include <cstring>
#include <limits>
#include <vector>

namespace dsfw::audio {
using internal::ffmpegError;

struct FfmpegAudioDecoder::Impl {
    AVFormatContext* fmtCtx = nullptr;
    AVCodecContext* codecCtx = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* frame = nullptr;

    int audioStreamIdx = -1;
    bool opened = false;
    bool eof = false;

    AudioFormatInfo info;
    double currentPosSec = 0.0;
    double totalDurationSec = 0.0;

    // Decoded buffer for streaming mode
    std::vector<uint8_t> decodedData;
    int64_t decodedPos = 0; // read cursor in frames

    void cleanup();
    double calcDuration() const;
    bool decodeNextFrame(std::vector<uint8_t>& outData, int64_t targetFrames);
};

void FfmpegAudioDecoder::Impl::cleanup() {
    if (frame) {
        av_frame_free(&frame);
        frame = nullptr;
    }
    if (packet) {
        av_packet_free(&packet);
        packet = nullptr;
    }
    if (codecCtx) {
        avcodec_free_context(&codecCtx);
        codecCtx = nullptr;
    }
    if (fmtCtx) {
        avformat_close_input(&fmtCtx);
        fmtCtx = nullptr;
    }
    decodedData.clear();
    decodedPos = 0;
    audioStreamIdx = -1;
    opened = false;
    eof = false;
    currentPosSec = 0.0;
    totalDurationSec = 0.0;
}

double FfmpegAudioDecoder::Impl::calcDuration() const {
    if (!fmtCtx || audioStreamIdx < 0)
        return 0.0;
    AVStream* stream = fmtCtx->streams[audioStreamIdx];
    if (stream->duration > 0) {
        return static_cast<double>(stream->duration) * av_q2d(stream->time_base);
    }
    if (fmtCtx->duration > 0) {
        return static_cast<double>(fmtCtx->duration) / AV_TIME_BASE;
    }
    return 0.0;
}

bool FfmpegAudioDecoder::Impl::decodeNextFrame(std::vector<uint8_t>& outData, int64_t targetFrames) {
    if (!fmtCtx || !codecCtx || eof)
        return false;

    int bps = bytesPerSample(info.sampleFormat);
    int channels = info.channelCount;

    while (static_cast<int64_t>(outData.size() / (bps * channels)) < targetFrames) {
        int readRet = av_read_frame(fmtCtx, packet);
        if (readRet < 0) {
            if (readRet == AVERROR_EOF) {
                eof = true;
                break;
            }
            av_packet_unref(packet);
            continue;
        }
        if (packet->stream_index != audioStreamIdx) {
            av_packet_unref(packet);
            continue;
        }
        int ret = avcodec_send_packet(codecCtx, packet);
        av_packet_unref(packet);
        if (ret < 0)
            continue;

        while (avcodec_receive_frame(codecCtx, frame) == 0) {
            // Copy raw frame data in source format (no resampling)
            int frameBytes = frame->nb_samples * channels * bps;
            if (av_sample_fmt_is_planar(codecCtx->sample_fmt)) {
                // Planar: deinterleave to interleaved
                std::vector<uint8_t> planarBuf(frameBytes);
                for (int ch = 0; ch < channels; ++ch) {
                    const uint8_t* src = frame->data[ch];
                    auto* dst = planarBuf.data() + ch * bps;
                    for (int s = 0; s < frame->nb_samples; ++s) {
                        std::memcpy(dst + s * channels * bps, src + s * bps, bps);
                    }
                }
                outData.insert(outData.end(), planarBuf.begin(), planarBuf.end());
            } else {
                // Interleaved: direct copy
                outData.insert(outData.end(), frame->data[0], frame->data[0] + frameBytes);
            }
            av_frame_unref(frame);
        }
    }

    return !outData.empty();
}

// ===== Public API =====

FfmpegAudioDecoder::FfmpegAudioDecoder() : d(std::make_unique<Impl>()) {
}

FfmpegAudioDecoder::~FfmpegAudioDecoder() {
    close();
}

FfmpegAudioDecoder::FfmpegAudioDecoder(FfmpegAudioDecoder&&) noexcept = default;
FfmpegAudioDecoder& FfmpegAudioDecoder::operator=(FfmpegAudioDecoder&&) noexcept = default;

dsfw::Result<AudioFormatInfo> FfmpegAudioDecoder::probe(const std::string& path) {
    AVFormatContext* probeCtx = nullptr;
    int ret = avformat_open_input(&probeCtx, path.c_str(), nullptr, nullptr);
    if (ret < 0) {
        return dsfw::Result<AudioFormatInfo>::Error("Failed to open file for probing: " + path + " (" +
                                                       ffmpegError(ret) + ")");
    }

    ret = avformat_find_stream_info(probeCtx, nullptr);
    if (ret < 0) {
        avformat_close_input(&probeCtx);
        return dsfw::Result<AudioFormatInfo>::Error("Failed to find stream info: " + ffmpegError(ret));
    }

    int streamIdx = av_find_best_stream(probeCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (streamIdx < 0) {
        avformat_close_input(&probeCtx);
        return dsfw::Result<AudioFormatInfo>::Error("No audio stream found");
    }

    auto* codecPar = probeCtx->streams[streamIdx]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);

    AudioFormatInfo info;
    info.sampleRate = codecPar->sample_rate;
    info.channelCount = codecPar->ch_layout.nb_channels;

    switch (codecPar->format) {
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_FLTP:
            info.sampleFormat = SampleFormat::Float32;
            break;
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S16P:
            info.sampleFormat = SampleFormat::Int16;
            break;
        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_S32P:
            info.sampleFormat = SampleFormat::Int32;
            break;
        default:
            // Default to Float32 for unknown formats
            info.sampleFormat = SampleFormat::Float32;
            break;
    }

    AVStream* stream = probeCtx->streams[streamIdx];
    if (stream->duration > 0) {
        info.durationSec = static_cast<double>(stream->duration) * av_q2d(stream->time_base);
        info.totalFrameCount = static_cast<int64_t>(info.durationSec * info.sampleRate);
    } else if (probeCtx->duration > 0) {
        info.durationSec = static_cast<double>(probeCtx->duration) / AV_TIME_BASE;
        info.totalFrameCount = static_cast<int64_t>(info.durationSec * info.sampleRate);
    } else {
        info.durationSec = 0.0;
        info.totalFrameCount = -1;
    }

    info.bitRate = static_cast<int>(codecPar->bit_rate / 1000);
    if (codec) {
        info.codecName = codec->name ? codec->name : "unknown";
    }
    if (probeCtx->iformat && probeCtx->iformat->name) {
        info.containerName = probeCtx->iformat->name;
    }

    avformat_close_input(&probeCtx);
    return info;
}

dsfw::Result<void> FfmpegAudioDecoder::open(const std::string& path) {
    close();

    int ret = avformat_open_input(&d->fmtCtx, path.c_str(), nullptr, nullptr);
    if (ret < 0) {
        return dsfw::Result<void>::Error("Failed to open file: " + path + " (" + ffmpegError(ret) + ")");
    }

    ret = avformat_find_stream_info(d->fmtCtx, nullptr);
    if (ret < 0) {
        d->cleanup();
        return dsfw::Result<void>::Error("Failed to find stream info: " + ffmpegError(ret));
    }

    d->audioStreamIdx = av_find_best_stream(d->fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (d->audioStreamIdx < 0) {
        d->cleanup();
        return dsfw::Result<void>::Error("No audio stream found");
    }

    auto* codecPar = d->fmtCtx->streams[d->audioStreamIdx]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec) {
        d->cleanup();
        return dsfw::Result<void>::Error("Unsupported codec");
    }

    d->codecCtx = avcodec_alloc_context3(codec);
    if (!d->codecCtx) {
        d->cleanup();
        return dsfw::Result<void>::Error("Failed to allocate codec context");
    }
    avcodec_parameters_to_context(d->codecCtx, codecPar);
    ret = avcodec_open2(d->codecCtx, codec, nullptr);
    if (ret < 0) {
        d->cleanup();
        return dsfw::Result<void>::Error("Failed to open codec: " + ffmpegError(ret));
    }

    // Fill format info
    d->info.sampleRate = codecPar->sample_rate;
    d->info.channelCount = codecPar->ch_layout.nb_channels;
    switch (codecPar->format) {
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_FLTP:
            d->info.sampleFormat = SampleFormat::Float32;
            break;
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S16P:
            d->info.sampleFormat = SampleFormat::Int16;
            break;
        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_S32P:
            d->info.sampleFormat = SampleFormat::Int32;
            break;
        default:
            d->info.sampleFormat = SampleFormat::Float32;
            break;
    }

    d->totalDurationSec = d->calcDuration();
    d->info.durationSec = d->totalDurationSec;
    if (d->totalDurationSec > 0.0) {
        d->info.totalFrameCount = static_cast<int64_t>(d->totalDurationSec * d->info.sampleRate);
    } else {
        d->info.totalFrameCount = -1;
    }

    d->info.bitRate = static_cast<int>(codecPar->bit_rate / 1000);
    if (codec && codec->name) {
        d->info.codecName = codec->name;
    }
    if (d->fmtCtx->iformat && d->fmtCtx->iformat->name) {
        d->info.containerName = d->fmtCtx->iformat->name;
    }

    d->packet = av_packet_alloc();
    d->frame = av_frame_alloc();
    d->opened = true;
    d->eof = false;
    d->currentPosSec = 0.0;

    return dsfw::Result<void>::Ok();
}

void FfmpegAudioDecoder::close() {
    d->cleanup();
}

bool FfmpegAudioDecoder::isOpen() const {
    return d->opened;
}

const AudioFormatInfo& FfmpegAudioDecoder::formatInfo() const {
    return d->info;
}

dsfw::Result<AudioBuffer> FfmpegAudioDecoder::decodeAll() {
    if (!d->opened) {
        return dsfw::Result<AudioBuffer>::Error("No file opened");
    }

    int bps = bytesPerSample(d->info.sampleFormat);
    int channels = d->info.channelCount;

    // Reserve approximate space
    std::vector<uint8_t> data;
    if (d->totalDurationSec > 0.0) {
        auto estimatedSize = static_cast<size_t>(d->totalDurationSec * d->info.sampleRate * channels * bps);
        data.reserve(estimatedSize);
    }

    d->decodeNextFrame(data, std::numeric_limits<int64_t>::max());

    if (data.empty()) {
        return AudioBuffer::create(0, channels, d->info.sampleFormat);
    }

    auto totalFrames = static_cast<int64_t>(data.size() / (bps * channels));
    return AudioBuffer::fromVector(std::move(data), totalFrames, channels, d->info.sampleFormat);
}

dsfw::Result<AudioBuffer> FfmpegAudioDecoder::decodeSegment(double startSec, double endSec) {
    if (!d->opened) {
        return dsfw::Result<AudioBuffer>::Error("No file opened");
    }

    auto seekResult = seekToTime(startSec);
    if (!seekResult) {
        return dsfw::Result<AudioBuffer>::Error(seekResult.error());
    }

    double duration = endSec - startSec;
    int bps = bytesPerSample(d->info.sampleFormat);
    int channels = d->info.channelCount;
    int64_t targetFrames = static_cast<int64_t>(duration * d->info.sampleRate);

    std::vector<uint8_t> data;
    data.reserve(static_cast<size_t>(targetFrames * channels * bps));

    d->decodeNextFrame(data, targetFrames);

    if (data.empty()) {
        return AudioBuffer::create(0, channels, d->info.sampleFormat);
    }

    int64_t totalFrames = static_cast<int64_t>(data.size() / (bps * channels));
    return AudioBuffer::fromVector(std::move(data), totalFrames, channels, d->info.sampleFormat);
}

dsfw::Result<AudioBuffer> FfmpegAudioDecoder::read(int64_t frameCount) {
    if (!d->opened) {
        return dsfw::Result<AudioBuffer>::Error("No file opened");
    }
    if (d->eof) {
        return AudioBuffer::create(0, d->info.channelCount, d->info.sampleFormat);
    }

    int bps = bytesPerSample(d->info.sampleFormat);
    int channels = d->info.channelCount;

    std::vector<uint8_t> data;
    data.reserve(static_cast<size_t>(frameCount * channels * bps));

    bool gotData = d->decodeNextFrame(data, frameCount);

    if (data.empty() && !gotData) {
        return AudioBuffer::create(0, channels, d->info.sampleFormat);
    }

    int64_t framesRead = static_cast<int64_t>(data.size() / (bps * channels));
    d->currentPosSec += static_cast<double>(framesRead) / d->info.sampleRate;

    return AudioBuffer::fromVector(std::move(data), framesRead, channels, d->info.sampleFormat);
}

dsfw::Result<void> FfmpegAudioDecoder::seekToTime(double sec) {
    if (!d->opened) {
        return dsfw::Result<void>::Error("No file opened");
    }

    AVStream* stream = d->fmtCtx->streams[d->audioStreamIdx];
    int64_t timestamp =
        av_rescale_q(static_cast<int64_t>(sec * AV_TIME_BASE), av_make_q(1, AV_TIME_BASE), stream->time_base);

    int ret = av_seek_frame(d->fmtCtx, d->audioStreamIdx, timestamp, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        return dsfw::Result<void>::Error("Failed to seek to time: " + std::to_string(sec) + " (" +
                                            ffmpegError(ret) + ")");
    }

    avcodec_flush_buffers(d->codecCtx);
    d->eof = false;
    d->currentPosSec = sec;
    d->decodedData.clear();
    d->decodedPos = 0;

    return dsfw::Result<void>::Ok();
}

double FfmpegAudioDecoder::currentTime() const {
    return d->currentPosSec;
}

double FfmpegAudioDecoder::totalDuration() const {
    return d->totalDurationSec;
}

} // namespace dsfw::audio