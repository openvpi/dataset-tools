#ifndef FFMPEGDECODER_H
#define FFMPEGDECODER_H

#include "Api/IAudioDecoder.h"

QSAPI_USING_NAMESPACE

class FFmpegDecoderPrivate;

class FFmpegDecoder final : public IAudioDecoder {
    Q_OBJECT
    Q_DECLARE_PRIVATE(FFmpegDecoder)
public:
    explicit FFmpegDecoder(QObject *parent = nullptr);
    ~FFmpegDecoder() override;

    bool open(const QString &filename, const QsMedia::WaveArguments &args = {}) override;
    void close() override;

    bool isOpen() const override;

    NAudio::WaveFormat inFormat() const override;

    NAudio::WaveFormat Format() const override;

    void SetPosition(qint64 pos) override;

    qint64 Position() const override;
    qint64 Length() const override;

    int Read(char *buffer, int offset, int count) override;
    int Read(float *buffer, int offset, int count) override;

protected:
    FFmpegDecoder(FFmpegDecoderPrivate &d, QObject *parent = nullptr);

    QScopedPointer<FFmpegDecoderPrivate> d_ptr;
};


#endif // FFMPEGDECODER_H
