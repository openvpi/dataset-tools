#include "FFmpegDecoder.h"

#include "private/FFmpegDecoder_p.h"

#include "QsMediaNamespace.h"

#include <QDebug>

NAUDIO_USING_NAMESPACE

static int extractInt(const QVariant &var, int defaultValue) {
    int res = defaultValue;
    switch (var.type()) {
        case QVariant::Int:
            res = var.toInt();
            break;
        case QVariant::UInt:
            res = var.toInt();
            break;
        case QVariant::LongLong:
            res = var.toLongLong();
            break;
        case QVariant::ULongLong:
            res = var.toULongLong();
            break;
        case QVariant::Double:
            res = var.toDouble();
            break;
        default:
            break;
    }
    return res;
}

FFmpegDecoder::FFmpegDecoder(QObject *parent) : FFmpegDecoder(*new FFmpegDecoderPrivate(), parent) {
}

FFmpegDecoder::~FFmpegDecoder() {
    FFmpegDecoder::close();
}

bool FFmpegDecoder::open(const QString &filename, const QsMedia::WaveArguments &args) {
    Q_D(FFmpegDecoder);

    if (d->isOpen) {
        close();
    }

    d->_fileName = filename;
    d->_arguments = FFmpegDecoderPrivate::WaveArguments(                   //
        args.sampleRate, (AVSampleFormat) args.sampleFormat, args.channels //
    );

    if (!d->initDecoder()) {
        return false;
    }

    return true;
}

void FFmpegDecoder::close() {
    Q_D(FFmpegDecoder);

    if (d->isOpen)
        d->quitDecoder();
}

bool FFmpegDecoder::isOpen() const {
    Q_D(const FFmpegDecoder);
    return d->isOpen;
}

WaveFormat FFmpegDecoder::inFormat() const {
    Q_D(const FFmpegDecoder);
    return d->_waveFormat;
}

WaveFormat FFmpegDecoder::Format() const {
    Q_D(const FFmpegDecoder);
    return d->_resampledFormat;
}

void FFmpegDecoder::SetPosition(qint64 pos) {
    Q_D(FFmpegDecoder);
    std::lock_guard locker(d->lockObject);
    d->_pos = d->dest2src_bytes(pos / d->_arguments.Channels);
    d->seek();
}

qint64 FFmpegDecoder::Position() const {
    Q_D(const FFmpegDecoder);
    return d->src2dest_bytes(d->_pos) * d->_arguments.Channels;
}

qint64 FFmpegDecoder::Length() const {
    Q_D(const FFmpegDecoder);
    return d->src2dest_bytes(d->_length) * d->_arguments.Channels;
}

int FFmpegDecoder::Read(char *buffer, int offset, int count) {
    Q_D(FFmpegDecoder);

    std::lock_guard locker(d->lockObject);
    int res = 0;

    if (offset > 0) {
        int err = d->decode(nullptr, offset);
        if (err < 0) {
            return err;
        }
    }

    if (count > 0) {
        res = d->decode(buffer, count);
    }

    return res;
}

int FFmpegDecoder::Read(float *buffer, int offset, int count) {
    Q_D(FFmpegDecoder);

    if (d->_arguments.SampleFormat != AV_SAMPLE_FMT_FLT) {
        qDebug() << "FFmpeg: Sample format should be FLT.";
        return -1;
    }

    std::lock_guard locker(d->lockObject);
    int res = 0;
    const int bytesPerSample = 4;

    offset *= bytesPerSample;
    count *= bytesPerSample;

    if (offset > 0) {
        int err = d->decode(nullptr, offset);
        if (err < 0) {
            return err;
        }
    }

    if (count > 0) {
        res = d->decode((char *) buffer, count);
    }

    res /= bytesPerSample;

    return res;
}

FFmpegDecoder::FFmpegDecoder(FFmpegDecoderPrivate &d, QObject *parent) : IAudioDecoder(parent), d_ptr(&d) {
    d.q_ptr = this;
    d.init();
}
