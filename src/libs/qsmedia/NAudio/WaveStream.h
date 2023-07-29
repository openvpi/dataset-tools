#ifndef WAVESTREAM_H
#define WAVESTREAM_H

#include <QIODevice>

#include "WaveFormat.h"

NAUDIO_BEGIN_NAMESPACE

class QSMEDIA_API WaveStream {
public:
    WaveStream();
    ~WaveStream();

    /**
     * @brief The reference used to determine audio position
     *        Override when inherited as a resampler
     * 
     * @return WaveFormat 
     */
    virtual WaveFormat Format() const = 0;
    virtual int BlockAlign() const;
    virtual void Flush();

    virtual int Read(char *buffer, int offset, int count) = 0;
    virtual int Read(float *buffer, int offset, int count);

    // Expressed in bytes
    virtual void SetPosition(qint64 pos) = 0;
    virtual qint64 Position() const = 0;
    virtual qint64 Length() const = 0;

    enum SeekOrigin {
        // These constants match Win32's FILE_BEGIN, FILE_CURRENT, and FILE_END
        Begin = 0,
        Current = 1,
        End = 2,
    };
    qint64 Seek(qint64 offset, SeekOrigin origin);

    // Expressed in milliseconds
    void Skip(qint64 msecs);
    qint64 CurrentTime() const;
    void SetCurrentTime(qint64 msecs);
    qint64 TotalTime() const;

    // Expressed in samples
    void SkipSamples(qint64 count);
    qint64 TotalSamples() const;
    void SetCurrentSample(qint64 count);
    qint64 CurrentSample() const;

    virtual bool HasData(int count) const;
};

NAUDIO_END_NAMESPACE

#endif // WAVESTREAM_H
