#include "FFmpegDecoder_p.h"

#include <QDebug>

NAUDIO_USING_NAMESPACE

FFmpegDecoderPrivate::FFmpegDecoderPrivate() {
}

FFmpegDecoderPrivate::~FFmpegDecoderPrivate() {
}

void FFmpegDecoderPrivate::init() {
    isOpen = false;

    clear();
}

void FFmpegDecoderPrivate::clear() {
    _formatContext = nullptr;
    _codecContext = nullptr;
    _swrContext = nullptr;
    _packet = nullptr;
    _frame = nullptr;

    _length = 0;
    _pos = 0;
    _audioIndex = -1;
    _remainSamples = 0;
    _cachedBufferPos = 0;

    _cachedBuffer.clear();
}

bool FFmpegDecoderPrivate::initDecoder() {
    auto fmt_ctx = avformat_alloc_context();

    // 打开文件
    auto ret = avformat_open_input(&fmt_ctx, _fileName.toStdString().data(), nullptr, nullptr);
    if (ret != 0) {
        avformat_free_context(fmt_ctx);

        qDebug() << QString("FFmpeg: Failed to load file %1.").arg(_fileName);
        return false;
    }

    _formatContext = fmt_ctx;

    // 查找流信息
    ret = avformat_find_stream_info(fmt_ctx, nullptr);
    if (ret < 0) {
        qDebug() << "FFmpeg: Failed to find streams.";
        return false;
    }

    // 查找音频流
    auto audio_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio_idx < 0) {
        qDebug() << "FFmpeg: Failed to find audio stream.";
        return false;
    }

    _audioIndex = audio_idx;

    // 查找解码器
    auto stream = fmt_ctx->streams[audio_idx];
    auto codec_param = stream->codecpar;
    auto codec = avcodec_find_decoder(codec_param->codec_id);
    if (!codec) {
        qDebug() << "FFmpeg: Failed to find decoder.";
        return false;
    }

    // 分配解码器上下文
    auto codec_ctx = avcodec_alloc_context3(nullptr);
    _codecContext = codec_ctx;

    // 传递解码器信息
    ret = avcodec_parameters_to_context(codec_ctx, codec_param);
    if (ret < 0) {
        qDebug() << "FFmpeg: Failed to pass params to codec.";
        return false;
    }

    // 没有此句会出现：Could not update timestamps for skipped samples
    codec_ctx->pkt_timebase = fmt_ctx->streams[audio_idx]->time_base;

    // 打开解码器
    ret = avcodec_open2(codec_ctx, codec, nullptr);
    if (ret < 0) {
        qDebug() << "FFmpeg: Failed to open decoder.";
        return false;
    }

    int srcBytesPerSample = av_get_bytes_per_sample(codec_ctx->sample_fmt);
    int srcChannels = codec_ctx->ch_layout.nb_channels;

    // 记录音频信息
    _length = (stream->duration * (stream->time_base.num / (double) stream->time_base.den) *
               codec_ctx->sample_rate * srcBytesPerSample);

    _waveFormat = WaveFormat(codec_ctx->sample_rate, srcBytesPerSample * 8, srcChannels);

    // 修改缺省参数
    if (_arguments.SampleRate < 0) {
        _arguments.SampleRate = codec_ctx->sample_rate;
        qDebug() << QString("FFmpeg: Set default sample rate as %1")
                        .arg(QString::number(_arguments.SampleRate));
    }

    if (_arguments.SampleFormat < 0) {
        _arguments.SampleFormat = codec_ctx->sample_fmt;
        qDebug() << QString("FFmpeg: Set default sample format as %1")
                        .arg(QString(av_get_sample_fmt_name(_arguments.SampleFormat)));
    }

    if (_arguments.Channels < 0) {
        _arguments.Channels = srcChannels > 2 ? 2 : srcChannels;
        qDebug() << QString("FFmpeg: Set default channels as %1")
                        .arg(QString::number(_arguments.Channels));
    }

    if (_arguments.Channels > 2) {
        qDebug() << "FFmpeg: Only support basic mono and stereo.";
        return false;
    }

    // 对外表现的格式
    if (_arguments.SampleFormat == AV_SAMPLE_FMT_FLT) {
        _resampledFormat =
            WaveFormat::CreateIeeeFloatWaveFormat(_arguments.SampleRate, _arguments.Channels);
    } else {
        _resampledFormat =
            WaveFormat(_arguments.SampleRate, _arguments.BytesPerSample() * 8, _arguments.Channels);
    }

    // 初始化默认输出声道结构
    AVChannelLayout out_ch_layout;
    av_channel_layout_default(&out_ch_layout, _arguments.Channels);
    ret = av_channel_layout_copy(&_channelLayout, &out_ch_layout);
    if (ret < 0) {
        av_channel_layout_uninit(&out_ch_layout);
        error_on_channel_copy(-ret);
        return false;
    }

    // 初始化重采样器
    auto swr = swr_alloc();
    _swrContext = swr;

    ret = swr_alloc_set_opts2(&swr, &out_ch_layout, _arguments.SampleFormat, _arguments.SampleRate,
                              &codec_ctx->ch_layout, codec_ctx->sample_fmt, codec_ctx->sample_rate,
                              0, nullptr);
    av_channel_layout_uninit(&out_ch_layout);

    if (ret != 0) {
        qDebug() << "FFmpeg: Failed to create resampler.";
        return false;
    }

    ret = swr_init(swr);
    if (ret < 0) {
        qDebug() << "FFmpeg: Failed to init resampler.";
        return false;
    }

    // 初始化数据包和数据帧
    auto pkt = av_packet_alloc();
    auto frame = av_frame_alloc();

    // 等待进一步的解码
    _packet = pkt;
    _frame = frame;

    isOpen = true;

    return true;
}

void FFmpegDecoderPrivate::quitDecoder() {
    auto fmt_ctx = _formatContext;
    auto codec_ctx = _codecContext;
    auto swr_ctx = _swrContext;

    auto pkt = _packet;
    auto frame = _frame;

    if (frame) {
        av_frame_free(&frame);
    }

    if (pkt) {
        av_packet_free(&pkt);
    }

    if (swr_ctx) {
        swr_close(swr_ctx);
        swr_free(&swr_ctx);
    }

    if (codec_ctx) {
        avcodec_close(codec_ctx);
        avcodec_free_context(&codec_ctx);
    }

    if (fmt_ctx) {
        avformat_close_input(&fmt_ctx);
    }

    av_channel_layout_uninit(&_channelLayout);

    clear();

    isOpen = false;
}

int FFmpegDecoderPrivate::decode(char *buf, int size) {
    auto fmt_ctx = _formatContext;
    auto codec_ctx = _codecContext;
    auto swr_ctx = _swrContext;

    auto pkt = _packet;
    auto frame = _frame;

    auto stream = fmt_ctx->streams[_audioIndex];

    // 准备声道信息
    AVChannelLayout out_ch_layout;
    int ret = av_channel_layout_copy(&out_ch_layout, &_channelLayout);
    if (ret < 0) {
        error_on_channel_copy(-ret);
        return -1;
    }

    // 跳过上次重采样器的余量
    int remained = 0;
    if (_remainSamples > 0) {
        // 获取采样数
        remained = (int) av_rescale_rnd(_remainSamples, _arguments.SampleRate,
                                        _waveFormat.SampleRate(), AV_ROUND_UP);
        // 获取字节数
        remained = remained == 0 ? 0
                                 : av_samples_get_buffer_size(nullptr, _channelLayout.nb_channels,
                                                              remained, _arguments.SampleFormat, 1);

        _remainSamples = 0;
    }

    int bufferWriteOffset = 0;

    // 采取边解码边写到输出缓冲区的方式。策略是先把cache全部写出，然后边解码边写，写到最后剩下的再存入cache
    auto cacheTransferSize = std::min(int(_cachedBuffer.size()) - _cachedBufferPos, size);
    if (cacheTransferSize > 0) {
        ::memcpy(buf, _cachedBuffer.data() + _cachedBufferPos, cacheTransferSize);
        _cachedBufferPos += cacheTransferSize;
        bufferWriteOffset = cacheTransferSize;
    }

    while (bufferWriteOffset < size) {
        int ret = av_read_frame(fmt_ctx, pkt);

        // 判断是否结束
        if (ret == AVERROR_EOF) {
            //标记为最终时间
            _pos = _length;

            av_packet_unref(pkt);
            break;
        } else if (ret != 0) {
            // 忽略
            qDebug() << QString("FFmpeg: Error getting next frame with code %1, ignored")
                            .arg(QString::number(-ret));
            continue;
        }

        // 跳过其他流
        if (pkt->stream_index != _audioIndex) {
            av_packet_unref(pkt);
            continue;
        }

        // 发送待解码包
        ret = avcodec_send_packet(codec_ctx, pkt);
        av_packet_unref(pkt);
        if (ret < 0) {
            // 忽略
            qDebug() << QString(
                            "FFmpeg: Error submitting a packet for decoding with code %1, ignored")
                            .arg(QString::number(-ret));
            continue;
        }

        while (ret >= 0) {
            // 接收解码数据
            ret = avcodec_receive_frame(codec_ctx, frame);
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                // 结束
                break;
            } else if (ret < 0) {
                // 出错
                av_frame_unref(frame);

                // 忽略
                qDebug() << QString("FFmpeg: Error decoding frame with code %1, ignored")
                                .arg(QString::number(-ret));
                continue;
            }

            // 记录当前时间
            _pos = (frame->best_effort_timestamp / (double) stream->duration * _length);

            // 进行重采样
            auto resampled_frame = av_frame_alloc();
            resampled_frame->sample_rate = _arguments.SampleRate;
            resampled_frame->format = (int) _arguments.SampleFormat;

            ret = av_channel_layout_copy(&resampled_frame->ch_layout, &out_ch_layout);
            if (ret < 0) {
                goto out_channel_copy;
            }

            ret = swr_convert_frame(swr_ctx, resampled_frame, frame);
            if (ret == 0) {
                auto &skip = remained;

                int sz = av_samples_get_buffer_size(nullptr, resampled_frame->ch_layout.nb_channels,
                                                    resampled_frame->nb_samples,
                                                    _arguments.SampleFormat, 1);
                auto arr = resampled_frame->data[0];

                if (sz >= skip) {
                    int size_need = size - bufferWriteOffset;
                    int size_supply = sz - skip;

                    // 写到输出缓冲区，写满了就存入cache
                    if (size_supply > size_need) {
                        // 写输出缓冲区
                        ::memcpy(buf + bufferWriteOffset, arr + skip, size_need);

                        // 存入cache
                        _cachedBuffer.resize(size_supply - size_need);
                        ::memcpy(_cachedBuffer.data(), arr + skip + size_need,
                                 size_supply - size_need);
                        _cachedBufferPos = 0;

                        bufferWriteOffset = size;
                    } else {
                        // 全部写到输出缓冲区
                        ::memcpy(buf + bufferWriteOffset, arr + skip, size_supply);
                        bufferWriteOffset += size_supply;
                    }
                    skip = 0;
                } else {
                    skip -= sz;
                }
            } else {
                // 忽略
                if (ret == AVERROR_INVALIDDATA) {
                    qDebug() << QString("FFmpeg: Error resampling frame with code %1, ignored")
                                    .arg(QString::number(-ret));
                } else {
                    goto out_resample;
                }
            }

            goto out_normal;

        out_channel_copy:
            av_frame_free(&resampled_frame);
            av_frame_unref(frame);
            av_channel_layout_uninit(&out_ch_layout);
            error_on_channel_copy(-ret);
            return -1;

        out_resample:
            av_frame_free(&resampled_frame);
            av_frame_unref(frame);
            av_channel_layout_uninit(&out_ch_layout);
            qDebug() << QString("FFmpeg: Error resampling frame with code %1")
                            .arg(QString::number(-ret));
            return -1;

        out_normal:
            av_frame_free(&resampled_frame);
            av_frame_unref(frame);
        }
    }

    av_channel_layout_uninit(&out_ch_layout);
    return bufferWriteOffset;
}

void FFmpegDecoderPrivate::seek() {
    auto fmt_ctx = _formatContext;
    auto swr_ctx = _swrContext;
    auto codec_ctx = _codecContext;

    _cachedBuffer.clear();

    // 必须清空内部缓存
    avcodec_flush_buffers(codec_ctx);

    auto stream = fmt_ctx->streams[_audioIndex];
    qint64 timestamp = _pos / (double) _length * stream->duration;

    int ret = av_seek_frame(fmt_ctx, _audioIndex, timestamp, AVSEEK_FLAG_FRAME);
    if (ret < 0) {
        qDebug() << QString("FFmpeg: Error seek frame with code %1").arg(QString::number(-ret));
    }

    // 保存重采样器内部余量
    _remainSamples = (int) swr_get_delay(swr_ctx, _waveFormat.SampleRate());
}

int FFmpegDecoderPrivate::orgBytesPerSample() const {
    return _waveFormat.BitsPerSample() / 8;
}

qint64 FFmpegDecoderPrivate::src2dest_bytes(qint64 bytes) const {
    return bytes / orgBytesPerSample() / _waveFormat.SampleRate() * _arguments.SampleRate *
           _arguments.BytesPerSample();
}

qint64 FFmpegDecoderPrivate::dest2src_bytes(qint64 bytes) const {
    return (bytes / (double) _arguments.BytesPerSample() / _arguments.SampleRate *
            _waveFormat.SampleRate() * orgBytesPerSample());
}

void FFmpegDecoderPrivate::error_on_channel_copy(int code) const {
    qDebug() << QString("FFmpeg: Copy channel layout with code %1").arg(QString::number(code));
}
