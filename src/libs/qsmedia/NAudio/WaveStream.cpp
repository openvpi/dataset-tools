#include "WaveStream.h"

NAUDIO_BEGIN_NAMESPACE

static void set_pos_bounded(WaveStream *stream, qint64 pos) {
    if (pos > stream->Length())
        stream->SetPosition(stream->Length());
    else if (pos < 0)
        stream->SetPosition(0);
    else
        stream->SetPosition(pos);
}

WaveStream::WaveStream() {
}

WaveStream::~WaveStream() {
}

int WaveStream::BlockAlign() const {
    return Format().BlockAlign();
}

void WaveStream::Flush() {
}

qint64 WaveStream::Seek(qint64 offset, WaveStream::SeekOrigin origin) {
    switch (origin) {
        case SeekOrigin::Begin:
            SetPosition(offset);
            break;
        case SeekOrigin::Current:
            SetPosition(Position() + offset);
            break;
        default:
            SetPosition(Length() + offset);
            break;
    }
    return Position();
}

void WaveStream::Skip(qint64 msecs) {
    qint64 num = Position() + qint64(Format().AverageBytesPerSecond() * double(msecs) / 1000);
    set_pos_bounded(this, num);
}

qint64 WaveStream::CurrentTime() const {
    return double(Position()) / Format().AverageBytesPerSecond() * 1000;
}

void WaveStream::SetCurrentTime(qint64 msecs) {
    qint64 num = double(msecs) * Format().AverageBytesPerSecond() / 1000;
    set_pos_bounded(this, num);
}

qint64 WaveStream::TotalTime() const {
    return double(Length()) / Format().AverageBytesPerSecond() * 1000;
}

void WaveStream::SkipSamples(qint64 count) {
    qint64 num = Position() + qint64(double(count) / int(Format().BitsPerSample() / 8));
    set_pos_bounded(this, num);
}

qint64 WaveStream::TotalSamples() const {
    return double(Length()) / int(Format().BitsPerSample() / 8);
}

void WaveStream::SetCurrentSample(qint64 count) {
    qint64 num = double(count) / int(Format().BitsPerSample() / 8);
    set_pos_bounded(this, num);
}

qint64 WaveStream::CurrentSample() const {
    return double(Position()) / int(Format().BitsPerSample() / 8);
}

int WaveStream::Read(float *buffer, int offset, int count) {
    return Read((char *) buffer, offset * 4, count * 4);
}

bool WaveStream::HasData(int count) const {
    Q_UNUSED(count);
    return Position() < Length();
}

NAUDIO_END_NAMESPACE