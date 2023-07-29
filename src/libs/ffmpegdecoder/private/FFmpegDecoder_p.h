#ifndef FFMPEGDECODERPRIVATE_H
#define FFMPEGDECODERPRIVATE_H

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

#include "../FFmpegDecoder.h"

#include <mutex>
#include <vector>

class FFmpegDecoderPrivate {
    Q_DECLARE_PUBLIC(FFmpegDecoder);

public:
    FFmpegDecoderPrivate();
    ~FFmpegDecoderPrivate();

    void init();

    void clear();

    bool initDecoder();
    void quitDecoder();

    int decode(char *buf, int size);
    void seek();

    FFmpegDecoder *q_ptr;

    bool isOpen;

    struct WaveArguments {
        int SampleRate;
        AVSampleFormat SampleFormat;
        int Channels;

        WaveArguments() : WaveArguments(-1, AV_SAMPLE_FMT_NONE, -1) {
        }

        WaveArguments(AVSampleFormat fmt) : WaveArguments(-1, fmt, -1) {
        }

        WaveArguments(int sampleRate, AVSampleFormat fmt, int channels) {
            SampleRate = sampleRate;
            SampleFormat = fmt;
            Channels = channels;
        }

        int BytesPerSample() const {
            return av_get_bytes_per_sample(SampleFormat);
        }
    };

    // FFmpeg 指针
    AVFormatContext *_formatContext;

    AVCodecContext *_codecContext;

    SwrContext *_swrContext;

    AVPacket *_packet;

    AVFrame *_frame;

    // 输入音频信息
    NAudio::WaveFormat _waveFormat;

    // 输出音频信息
    NAudio::WaveFormat _resampledFormat;

    // 文件名参数
    QString _fileName;

    // 输出参数
    WaveArguments _arguments;

    // 音频信息
    qint64 _length; // 单个声道总字节数

    qint64 _pos; // 单个声道已处理的字节数

    int _audioIndex; // 音频流序号

    AVChannelLayout _channelLayout; // 输出声道布局

    // 类内数据结构
    std::vector<char> _cachedBuffer; // 内部缓冲区

    int _cachedBufferPos; // 内部缓冲区读取位置

    int _remainSamples; // 重采样器余量

    std::mutex lockObject;

    int orgBytesPerSample() const;

    qint64 src2dest_bytes(qint64 bytes) const;

    qint64 dest2src_bytes(qint64 bytes) const;

    void error_on_channel_copy(int code) const;
};

#endif // FFMPEGDECODERPRIVATE_H
