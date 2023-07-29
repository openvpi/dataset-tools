#include "WaveFormat.h"

#include "WaveFormatExtraData.h"

NAUDIO_BEGIN_NAMESPACE

template <class T>
static QByteArray toBytes(T val) {
    return QByteArray((char *) (&val), sizeof(T));
}

template <class T>
static T fromBytes(QIODevice *d) {
    QByteArray data = d->read(sizeof(T));
    return *((T *) data.data());
}

WaveFormat::WaveFormat(int rate, int bits, int channels) {
    this->waveFormatTag = NAudio::Pcm;
    this->channels = (short) channels;
    this->sampleRate = rate;
    this->bitsPerSample = (short) bits;
    this->extraSize = (short) 0;
    this->blockAlign = (short) (channels * (bits / 8));
    this->averageBytesPerSecond = this->sampleRate * (int) this->blockAlign;
}

int WaveFormat::ConvertLatencyToByteSize(int milliseconds) const {
    int byteSize = (int) ((double) AverageBytesPerSecond() / 1000.0 * (double) milliseconds);
    if (byteSize % BlockAlign() != 0)
        byteSize = byteSize + BlockAlign() - byteSize % BlockAlign();
    return byteSize;
}

WaveFormat WaveFormat::CreateCustomFormat(NAudio::WaveFormatEncoding tag, int sampleRate,
                                          int channels, int averageBytesPerSecond, int blockAlign,
                                          int bitsPerSample) {
    WaveFormat fmt;
    fmt.waveFormatTag = tag;
    fmt.channels = channels;
    fmt.sampleRate = sampleRate;
    fmt.averageBytesPerSecond = averageBytesPerSecond;
    fmt.blockAlign = blockAlign;
    fmt.bitsPerSample = bitsPerSample;
    fmt.extraSize = 0;
    return fmt;
}

WaveFormat WaveFormat::CreateALawFormat(int sampleRate, int channels) {
    return WaveFormat::CreateCustomFormat(NAudio::ALaw, sampleRate, channels, sampleRate * channels,
                                          channels, 8);
}

WaveFormat WaveFormat::CreateMuLawFormat(int sampleRate, int channels) {
    return WaveFormat::CreateCustomFormat(NAudio::MuLaw, sampleRate, channels,
                                          sampleRate * channels, channels, 8);
}

WaveFormat WaveFormat::CreateIeeeFloatWaveFormat(int sampleRate, int channels) {
    WaveFormat ieeeFloatWaveFormat;
    ieeeFloatWaveFormat.waveFormatTag = NAudio::IeeeFloat;
    ieeeFloatWaveFormat.channels = channels;
    ieeeFloatWaveFormat.bitsPerSample = 32;
    ieeeFloatWaveFormat.sampleRate = sampleRate;
    ieeeFloatWaveFormat.blockAlign = 4 * channels;

    ieeeFloatWaveFormat.averageBytesPerSecond = sampleRate * ieeeFloatWaveFormat.blockAlign;
    ieeeFloatWaveFormat.extraSize = 0;
    return ieeeFloatWaveFormat;
}

WaveFormat WaveFormat::FromFormatChunk(QIODevice *br, int formatChunkLength) {
    WaveFormatExtraData waveFormatExtraData;
    waveFormatExtraData.ReadWaveFormat(br, formatChunkLength);
    waveFormatExtraData.ReadExtraData(br);
    return (WaveFormat) waveFormatExtraData;
}

#define BinaryRead(TYPE, BR) (*((TYPE *) BR->read(sizeof(TYPE)).data()))

bool WaveFormat::ReadWaveFormat(QIODevice *br, int formatChunkLength) {
    if (formatChunkLength < 16) {
        return false;
    }
    this->waveFormatTag = (NAudio::WaveFormatEncoding) fromBytes<quint32>(br);
    this->channels = fromBytes<quint16>(br);
    this->sampleRate = fromBytes<qint32>(br);
    this->averageBytesPerSecond = fromBytes<qint32>(br);
    this->blockAlign = fromBytes<qint16>(br);
    this->bitsPerSample = fromBytes<qint16>(br);
    if (formatChunkLength <= 16)
        return true;
    this->extraSize = fromBytes<qint16>(br);
    if ((int) this->extraSize == formatChunkLength - 18)
        return true;
    this->extraSize = (short) (formatChunkLength - 18);
    return false;
}

WaveFormat::WaveFormat(QIODevice *br) {
    int formatChunkLength = fromBytes<qint32>(br);
    this->ReadWaveFormat(br, formatChunkLength);
}

QString WaveFormat::toString() const {
    switch (this->waveFormatTag) {
        case NAudio::Pcm:
        case NAudio::Extensible:
            return QString::asprintf("%d bit PCM: %dHz %d channels", this->bitsPerSample,
                                     this->sampleRate, this->channels);
        case NAudio::IeeeFloat:
            return QString::asprintf("%d bit IEEFloat: %dHz %d channels", this->bitsPerSample,
                                     this->sampleRate, this->channels);
        default:
            return "NAudio.WaveFormat " + QString::number(waveFormatTag);
    }
}

bool WaveFormat::operator==(const WaveFormat &waveFormat) const {
    return this->waveFormatTag == waveFormat.waveFormatTag &&
           (int) this->channels == (int) waveFormat.channels &&
           this->sampleRate == waveFormat.sampleRate &&
           this->averageBytesPerSecond == waveFormat.averageBytesPerSecond &&
           (int) this->blockAlign == (int) waveFormat.blockAlign &&
           (int) this->bitsPerSample == (int) waveFormat.bitsPerSample;
}

int WaveFormat::toHashCode() const {
    return (this->waveFormatTag ^ (NAudio::WaveFormatEncoding) this->channels ^
            (NAudio::WaveFormatEncoding) this->sampleRate ^
            (NAudio::WaveFormatEncoding) this->averageBytesPerSecond ^
            (NAudio::WaveFormatEncoding) this->blockAlign ^
            (NAudio::WaveFormatEncoding) this->bitsPerSample);
}

NAudio::WaveFormatEncoding WaveFormat::Encoding() const {
    return this->waveFormatTag;
}

void WaveFormat::Serialize(QIODevice *writer) {
    writer->write(toBytes<int>(18 + (int) this->extraSize));
    writer->write(toBytes<short>(this->Encoding()));
    writer->write(toBytes<short>(this->Channels()));
    writer->write(toBytes<int>(SampleRate()));
    writer->write(toBytes<int>(AverageBytesPerSecond()));
    writer->write(toBytes<short>(this->BlockAlign()));
    writer->write(toBytes<short>(this->BitsPerSample()));
    writer->write(toBytes<short>(this->extraSize));
}

NAUDIO_END_NAMESPACE