#ifndef WAVEFORMAT_H
#define WAVEFORMAT_H

#include <QIODevice>
#include <QString>

#include "QsMediaGlobal.h"
#include "WaveFormatEncoding.h"

NAUDIO_BEGIN_NAMESPACE

class QSMEDIA_API WaveFormat {
protected:
    NAudio::WaveFormatEncoding waveFormatTag;
    short channels;
    int sampleRate;
    int averageBytesPerSecond;
    short blockAlign;
    short bitsPerSample;
    short extraSize;

public:
    WaveFormat() : WaveFormat(44100, 16, 2) {
    }

    WaveFormat(int sampleRate, int channels) : WaveFormat(sampleRate, 16, channels) {
    }

    WaveFormat(int rate, int bits, int channels);

    int ConvertLatencyToByteSize(int milliseconds) const;

    static WaveFormat CreateCustomFormat(NAudio::WaveFormatEncoding tag, int sampleRate,
                                         int channels, int averageBytesPerSecond, int blockAlign,
                                         int bitsPerSample);

    static WaveFormat CreateALawFormat(int sampleRate, int channels);

    static WaveFormat CreateMuLawFormat(int sampleRate, int channels);

    static WaveFormat CreateIeeeFloatWaveFormat(int sampleRate, int channels);

    static WaveFormat FromFormatChunk(QIODevice *br, int formatChunkLength);

protected:
    bool ReadWaveFormat(QIODevice *br, int formatChunkLength);

public:
    WaveFormat(QIODevice *br);

    QString toString() const;

    bool operator==(const WaveFormat &waveFormat) const;

    int toHashCode() const;

    NAudio::WaveFormatEncoding Encoding() const;

    virtual void Serialize(QIODevice *writer);

    inline int Channels() const {
        return channels;
    };

    inline int SampleRate() const {
        return sampleRate;
    }

    inline int AverageBytesPerSecond() const {
        return averageBytesPerSecond;
    }

    inline virtual int BlockAlign() const {
        return blockAlign;
    }

    inline int BitsPerSample() const {
        return bitsPerSample;
    }

    inline int ExtraSize() const {
        return extraSize;
    }
};

NAUDIO_END_NAMESPACE

#endif // WAVEFORMAT_H
