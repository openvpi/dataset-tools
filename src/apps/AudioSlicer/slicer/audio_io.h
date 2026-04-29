#ifndef AUDIO_SLICER_AUDIO_IO_H
#define AUDIO_SLICER_AUDIO_IO_H

#include <QString>
#include <memory>

class IAudioReader {
public:
    virtual ~IAudioReader() = default;
    virtual qint64 frames() const = 0;
    virtual int channels() const = 0;
    virtual int sampleRate() const = 0;
    virtual bool seek(qint64 frame) = 0;
    virtual qint64 read(double *buffer, qint64 count) = 0;
    virtual int errorCode() const = 0;
    virtual QString errorMessage() const = 0;
};

class IAudioWriter {
public:
    virtual ~IAudioWriter() = default;
    virtual qint64 write(const double *buffer, qint64 count) = 0;
    virtual int errorCode() const = 0;
    virtual QString errorMessage() const = 0;
};

enum class OutputSampleFormat { PCM16, PCM24, PCM32, Float32 };

std::unique_ptr<IAudioReader> createAudioReader(const QString &filePath);
std::unique_ptr<IAudioWriter> createWavWriter(
    const QString &filePath, int sampleRate, int channels,
    OutputSampleFormat format);

#endif //AUDIO_SLICER_AUDIO_IO_H
