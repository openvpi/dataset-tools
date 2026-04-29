#ifndef AUDIO_SLICER_AUDIO_IO_SNDFILE_P_H
#define AUDIO_SLICER_AUDIO_IO_SNDFILE_P_H

#include <sndfile.hh>

#include "audio_io.h"

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32)) || (defined(UNICODE) || defined(_UNICODE))
#    define USE_WIDE_CHAR
#endif

class SndfileReader : public IAudioReader {
public:
    explicit SndfileReader(const QString &filePath);

    qint64 frames() const override;
    int channels() const override;
    int sampleRate() const override;
    bool seek(qint64 frame) override;
    qint64 read(double *buffer, qint64 count) override;
    int errorCode() const override;
    QString errorMessage() const override;

private:
    SndfileHandle m_handle;
};

class SndfileWriter : public IAudioWriter {
public:
    SndfileWriter(const QString &filePath, int sampleRate, int channels, OutputSampleFormat format);

    qint64 write(const double *buffer, qint64 count) override;
    int errorCode() const override;
    QString errorMessage() const override;

private:
    SndfileHandle m_handle;
};

#endif //AUDIO_SLICER_AUDIO_IO_SNDFILE_P_H
