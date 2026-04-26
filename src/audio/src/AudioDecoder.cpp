#include <dstools/AudioDecoder.h>
#include "FFmpegDecoder.h"

namespace dstools::audio {

struct AudioDecoder::Impl {
    FFmpegDecoder *decoder = nullptr;
};

AudioDecoder::AudioDecoder() : d(std::make_unique<Impl>()) {
    d->decoder = new FFmpegDecoder();
}

AudioDecoder::~AudioDecoder() {
    if (d->decoder) {
        if (d->decoder->isOpen()) {
            d->decoder->close();
        }
        delete d->decoder;
    }
}

bool AudioDecoder::open(const QString &path) {
    if (!d->decoder) return false;
    return d->decoder->open(path);
}

void AudioDecoder::close() {
    if (d->decoder) d->decoder->close();
}

bool AudioDecoder::isOpen() const {
    return d->decoder && d->decoder->isOpen();
}

WaveFormat AudioDecoder::format() const {
    if (!d->decoder || !d->decoder->isOpen()) return {};
    auto fmt = d->decoder->Format();
    return WaveFormat(fmt.SampleRate(), fmt.BitsPerSample(), fmt.Channels());
}

int AudioDecoder::read(char *buffer, int offset, int count) {
    if (!d->decoder) return 0;
    return d->decoder->Read(buffer, offset, count);
}

int AudioDecoder::read(float *buffer, int offset, int count) {
    if (!d->decoder) return 0;
    return d->decoder->Read(buffer, offset, count);
}

void AudioDecoder::setPosition(qint64 pos) {
    if (d->decoder) d->decoder->SetPosition(pos);
}

qint64 AudioDecoder::position() const {
    return d->decoder ? d->decoder->Position() : 0;
}

qint64 AudioDecoder::length() const {
    return d->decoder ? d->decoder->Length() : 0;
}

} // namespace dstools::audio
