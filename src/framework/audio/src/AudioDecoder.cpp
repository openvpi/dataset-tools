#include <dstools/AudioDecoder.h>

#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

#include <QDebug>
#include <cstring>
#include <vector>

namespace dstools::audio {

struct AudioDecoder::Impl {
    AVFormatContext *fmtCtx = nullptr;
    AVCodecContext *codecCtx = nullptr;
    SwrContext *swrCtx = nullptr;
    AVPacket *packet = nullptr;
    AVFrame *frame = nullptr;

    int audioStreamIdx = -1;
    bool opened = false;
    bool streaming = false;

    // Output format: always 44100 Hz, stereo, float32
    int outSampleRate = 44100;
    int outChannels = 2;
    int outBitsPerSample = 32; // float

    qint64 totalBytes = 0;   // total decoded length in bytes (float samples)
    qint64 currentPos = 0;   // current position in bytes

    // Decoded buffer
    std::vector<float> decodedData;
    qint64 decodedPos = 0; // read cursor into decodedData (in floats)

    bool decodeAll();
    bool decodeMore(size_t targetFloats);
    void cleanup();
    double calcDuration() const;
};

bool AudioDecoder::Impl::decodeAll() {
    decodedData.clear();
    decodedPos = 0;

    AVStream *stream = fmtCtx->streams[audioStreamIdx];
    if (stream->duration > 0) {
        double durationSec = static_cast<double>(stream->duration) * av_q2d(stream->time_base);
        if (durationSec > 0.0) {
            auto estimated = static_cast<size_t>(durationSec * outSampleRate * outChannels);
            decodedData.reserve(estimated + outSampleRate * outChannels);
        }
    }

    while (av_read_frame(fmtCtx, packet) >= 0) {
        if (packet->stream_index != audioStreamIdx) {
            av_packet_unref(packet);
            continue;
        }
        int ret = avcodec_send_packet(codecCtx, packet);
        av_packet_unref(packet);
        if (ret < 0) continue;

        while (avcodec_receive_frame(codecCtx, frame) == 0) {
            // Resample frame to output format
            int outSamples = swr_get_out_samples(swrCtx, frame->nb_samples);
            std::vector<float> buf(outSamples * outChannels);
            uint8_t *outBuf = reinterpret_cast<uint8_t *>(buf.data());

            int converted = swr_convert(swrCtx, &outBuf, outSamples,
                                        const_cast<const uint8_t **>(frame->extended_data),
                                        frame->nb_samples);
            if (converted > 0) {
                decodedData.insert(decodedData.end(), buf.begin(),
                                   buf.begin() + converted * outChannels);
            }
            av_frame_unref(frame);
        }
    }

    // Flush resampler
    {
        int outSamples = swr_get_out_samples(swrCtx, 0);
        if (outSamples > 0) {
            std::vector<float> buf(outSamples * outChannels);
            uint8_t *outBuf = reinterpret_cast<uint8_t *>(buf.data());
            int converted = swr_convert(swrCtx, &outBuf, outSamples, nullptr, 0);
            if (converted > 0) {
                decodedData.insert(decodedData.end(), buf.begin(),
                                   buf.begin() + converted * outChannels);
            }
        }
    }

    totalBytes = static_cast<qint64>(decodedData.size()) * sizeof(float);
    currentPos = 0;
    return !decodedData.empty();
}

void AudioDecoder::Impl::cleanup() {
    if (swrCtx) { swr_free(&swrCtx); swrCtx = nullptr; }
    if (frame) { av_frame_free(&frame); frame = nullptr; }
    if (packet) { av_packet_free(&packet); packet = nullptr; }
    if (codecCtx) { avcodec_free_context(&codecCtx); codecCtx = nullptr; }
    if (fmtCtx) { avformat_close_input(&fmtCtx); fmtCtx = nullptr; }
    decodedData.clear();
    decodedPos = 0;
    totalBytes = 0;
    currentPos = 0;
    audioStreamIdx = -1;
    streaming = false;
    opened = false;
}

double AudioDecoder::Impl::calcDuration() const {
    if (!fmtCtx || audioStreamIdx < 0) return 0.0;
    AVStream *stream = fmtCtx->streams[audioStreamIdx];
    if (stream->duration > 0) {
        return static_cast<double>(stream->duration) * av_q2d(stream->time_base);
    }
    // Fallback: estimate from format duration
    if (fmtCtx->duration > 0) {
        return static_cast<double>(fmtCtx->duration) / AV_TIME_BASE;
    }
    return 0.0;
}

bool AudioDecoder::Impl::decodeMore(size_t targetFloats) {
    if (!fmtCtx || !codecCtx) return false;

    while (decodedData.size() < targetFloats) {
        int readRet = av_read_frame(fmtCtx, packet);
        if (readRet < 0) {
            // EOF or error
            if (readRet == AVERROR_EOF) break;
            av_packet_unref(packet);
            continue;
        }
        if (packet->stream_index != audioStreamIdx) {
            av_packet_unref(packet);
            continue;
        }
        int ret = avcodec_send_packet(codecCtx, packet);
        av_packet_unref(packet);
        if (ret < 0) continue;

        while (avcodec_receive_frame(codecCtx, frame) == 0) {
            int outSamples = swr_get_out_samples(swrCtx, frame->nb_samples);
            std::vector<float> buf(outSamples * outChannels);
            uint8_t *outBuf = reinterpret_cast<uint8_t *>(buf.data());

            int converted = swr_convert(swrCtx, &outBuf, outSamples,
                                        const_cast<const uint8_t **>(frame->extended_data),
                                        frame->nb_samples);
            if (converted > 0) {
                decodedData.insert(decodedData.end(), buf.begin(),
                                   buf.begin() + converted * outChannels);
            }
            av_frame_unref(frame);
        }
    }

    totalBytes = static_cast<qint64>(decodedData.size()) * sizeof(float);
    return !decodedData.empty();
}

AudioDecoder::AudioDecoder() : d(std::make_unique<Impl>()) {
}

AudioDecoder::~AudioDecoder() {
    close();
}

bool AudioDecoder::open(const QString &path) {
    close();

    std::string pathStr = path.toUtf8().toStdString();

    if (avformat_open_input(&d->fmtCtx, pathStr.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "AudioDecoder: Failed to open " << pathStr << std::endl;
        return false;
    }

    if (avformat_find_stream_info(d->fmtCtx, nullptr) < 0) {
        std::cerr << "AudioDecoder: Failed to find stream info" << std::endl;
        d->cleanup();
        return false;
    }

    d->audioStreamIdx = av_find_best_stream(d->fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (d->audioStreamIdx < 0) {
        std::cerr << "AudioDecoder: No audio stream found" << std::endl;
        d->cleanup();
        return false;
    }

    auto *codecPar = d->fmtCtx->streams[d->audioStreamIdx]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec) {
        std::cerr << "AudioDecoder: Unsupported codec" << std::endl;
        d->cleanup();
        return false;
    }

    d->codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(d->codecCtx, codecPar);
    if (avcodec_open2(d->codecCtx, codec, nullptr) < 0) {
        std::cerr << "AudioDecoder: Failed to open codec" << std::endl;
        d->cleanup();
        return false;
    }

    // Setup resampler: input format -> float32 stereo 44100
    AVChannelLayout outChLayout{};
    av_channel_layout_default(&outChLayout, d->outChannels);
    int ret = swr_alloc_set_opts2(&d->swrCtx,
                                   &outChLayout, AV_SAMPLE_FMT_FLT, d->outSampleRate,
                                   &d->codecCtx->ch_layout, d->codecCtx->sample_fmt,
                                   d->codecCtx->sample_rate, 0, nullptr);
    if (ret < 0 || swr_init(d->swrCtx) < 0) {
        std::cerr << "AudioDecoder: Failed to init resampler" << std::endl;
        d->cleanup();
        return false;
    }

    d->packet = av_packet_alloc();
    d->frame = av_frame_alloc();

    if (!d->decodeAll()) {
        std::cerr << "AudioDecoder: Failed to decode audio data" << std::endl;
        d->cleanup();
        return false;
    }

    d->opened = true;
    return true;
}

bool AudioDecoder::openStreaming(const QString &path) {
    close();

    std::string pathStr = path.toUtf8().toStdString();

    if (avformat_open_input(&d->fmtCtx, pathStr.c_str(), nullptr, nullptr) < 0) {
        std::cerr << "AudioDecoder: Failed to open " << pathStr << std::endl;
        return false;
    }

    if (avformat_find_stream_info(d->fmtCtx, nullptr) < 0) {
        std::cerr << "AudioDecoder: Failed to find stream info" << std::endl;
        d->cleanup();
        return false;
    }

    d->audioStreamIdx = av_find_best_stream(d->fmtCtx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (d->audioStreamIdx < 0) {
        std::cerr << "AudioDecoder: No audio stream found" << std::endl;
        d->cleanup();
        return false;
    }

    auto *codecPar = d->fmtCtx->streams[d->audioStreamIdx]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecPar->codec_id);
    if (!codec) {
        std::cerr << "AudioDecoder: Unsupported codec" << std::endl;
        d->cleanup();
        return false;
    }

    d->codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(d->codecCtx, codecPar);
    if (avcodec_open2(d->codecCtx, codec, nullptr) < 0) {
        std::cerr << "AudioDecoder: Failed to open codec" << std::endl;
        d->cleanup();
        return false;
    }

    // Setup resampler: input format -> float32 stereo 44100
    AVChannelLayout outChLayout{};
    av_channel_layout_default(&outChLayout, d->outChannels);
    int ret = swr_alloc_set_opts2(&d->swrCtx,
                                   &outChLayout, AV_SAMPLE_FMT_FLT, d->outSampleRate,
                                   &d->codecCtx->ch_layout, d->codecCtx->sample_fmt,
                                   d->codecCtx->sample_rate, 0, nullptr);
    if (ret < 0 || swr_init(d->swrCtx) < 0) {
        std::cerr << "AudioDecoder: Failed to init resampler" << std::endl;
        d->cleanup();
        return false;
    }

    d->packet = av_packet_alloc();
    d->frame = av_frame_alloc();

    d->streaming = true;
    d->opened = true;
    d->totalBytes = -1; // unknown until fully decoded
    return true;
}

void AudioDecoder::close() {
    d->cleanup();
}

bool AudioDecoder::isOpen() const {
    return d->opened;
}

WaveFormat AudioDecoder::format() const {
    if (!d->opened) return {};
    return WaveFormat(d->outSampleRate, d->outBitsPerSample, d->outChannels);
}

double AudioDecoder::totalDuration() const {
    if (!d->opened) return 0.0;
    return d->calcDuration();
}

int AudioDecoder::sampleRate() const {
    return d->outSampleRate;
}

int AudioDecoder::read(char *buffer, int offset, int count) {
    if (!d->opened) return 0;

    if (d->streaming) {
        // Streaming mode: decode on-demand
        qint64 floatNeeded = (d->currentPos + count + sizeof(float) - 1) / sizeof(float);
        if (static_cast<size_t>(floatNeeded) > d->decodedData.size()) {
            d->decodeMore(static_cast<size_t>(floatNeeded) + d->outSampleRate * d->outChannels);
        }
    }

    if (d->decodedData.empty()) return 0;

    const auto *src = reinterpret_cast<const char *>(d->decodedData.data());
    qint64 totalBytesAvail = static_cast<qint64>(d->decodedData.size()) * sizeof(float);
    qint64 remaining = totalBytesAvail - d->currentPos;
    if (remaining <= 0) return 0;

    int toRead = static_cast<int>(qMin(static_cast<qint64>(count), remaining));
    memcpy(buffer + offset, src + d->currentPos, toRead);
    d->currentPos += toRead;
    return toRead;
}

int AudioDecoder::read(float *buffer, int offset, int count) {
    if (!d->opened) return 0;

    if (d->streaming) {
        // Streaming mode: decode on-demand
        qint64 floatPos = d->currentPos / sizeof(float);
        size_t floatNeeded = static_cast<size_t>(floatPos + count);
        if (floatNeeded > d->decodedData.size()) {
            d->decodeMore(floatNeeded + d->outSampleRate * d->outChannels);
        }
    }

    if (d->decodedData.empty()) return 0;

    qint64 floatPos = d->currentPos / sizeof(float);
    qint64 totalFloats = static_cast<qint64>(d->decodedData.size());
    qint64 remaining = totalFloats - floatPos;
    if (remaining <= 0) return 0;

    int toRead = static_cast<int>(qMin(static_cast<qint64>(count), remaining));
    memcpy(buffer + offset, d->decodedData.data() + floatPos, toRead * sizeof(float));
    d->currentPos += toRead * sizeof(float);
    return toRead;
}

void AudioDecoder::setPosition(qint64 pos) {
    if (!d->opened) return;
    if (d->streaming) {
        // Streaming mode: seek within already-decoded buffer if possible
        // For seeking beyond decoded data, use seekToTime
        qint64 totalBytesAvail = static_cast<qint64>(d->decodedData.size()) * sizeof(float);
        d->currentPos = qBound(qint64(0), pos, totalBytesAvail);
        return;
    }
    qint64 totalBytesAvail = static_cast<qint64>(d->decodedData.size()) * sizeof(float);
    d->currentPos = qBound(qint64(0), pos, totalBytesAvail);
}

void AudioDecoder::seekToTime(double sec) {
    if (!d->opened) return;

    if (d->streaming && d->fmtCtx && d->audioStreamIdx >= 0) {
        // Streaming mode: seek in file, then clear buffer and decode from new position
        AVStream *stream = d->fmtCtx->streams[d->audioStreamIdx];
        int64_t timestamp = static_cast<int64_t>(sec / av_q2d(stream->time_base));
        int ret = av_seek_frame(d->fmtCtx, d->audioStreamIdx, timestamp, AVSEEK_FLAG_BACKWARD);
        if (ret >= 0) {
            avcodec_flush_buffers(d->codecCtx);
            d->decodedData.clear();
            d->currentPos = 0;
        }
    } else {
        // Non-streaming mode: calculate byte offset from time
        double duration = totalDuration();
        if (duration > 0.0) {
            qint64 totalBytes = static_cast<qint64>(d->decodedData.size()) * sizeof(float);
            qint64 pos = static_cast<qint64>(sec / duration * totalBytes);
            // Align to frame boundary (4 bytes = sizeof(float))
            pos = (pos / sizeof(float)) * sizeof(float);
            setPosition(pos);
        }
    }
}

qint64 AudioDecoder::position() const {
    return d->currentPos;
}

qint64 AudioDecoder::length() const {
    return d->totalBytes;
}

} // namespace dstools::audio
