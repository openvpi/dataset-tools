#include "audio_io_sndfile_p.h"

static int mapOutputFormat(OutputSampleFormat format) {
    switch (format) {
        case OutputSampleFormat::PCM16:  return SF_FORMAT_PCM_16;
        case OutputSampleFormat::PCM24:  return SF_FORMAT_PCM_24;
        case OutputSampleFormat::PCM32:  return SF_FORMAT_PCM_32;
        case OutputSampleFormat::Float32: return SF_FORMAT_FLOAT;
    }
    return SF_FORMAT_PCM_16;
}

SndfileReader::SndfileReader(const QString &filePath) {
#ifdef USE_WIDE_CHAR
    auto pathStr = filePath.toStdWString();
#else
    auto pathStr = filePath.toStdString();
#endif
    m_handle = SndfileHandle(pathStr.c_str());
}

qint64 SndfileReader::frames() const { return m_handle.frames(); }
int SndfileReader::channels() const { return m_handle.channels(); }
int SndfileReader::sampleRate() const { return m_handle.samplerate(); }

bool SndfileReader::seek(qint64 frame) {
    return m_handle.seek(frame, SEEK_SET) >= 0;
}

qint64 SndfileReader::read(double *buffer, qint64 count) {
    return m_handle.read(buffer, count);
}

int SndfileReader::errorCode() const { return m_handle.error(); }
QString SndfileReader::errorMessage() const { return QString::fromUtf8(m_handle.strError()); }

SndfileWriter::SndfileWriter(const QString &filePath, int sampleRate, int channels, OutputSampleFormat format) {
#ifdef USE_WIDE_CHAR
    auto pathStr = filePath.toStdWString();
#else
    auto pathStr = filePath.toStdString();
#endif
    m_handle = SndfileHandle(pathStr.c_str(), SFM_WRITE,
                             SF_FORMAT_WAV | mapOutputFormat(format),
                             channels, sampleRate);
}

qint64 SndfileWriter::write(const double *buffer, qint64 count) {
    return m_handle.write(buffer, count);
}

int SndfileWriter::errorCode() const { return m_handle.error(); }
QString SndfileWriter::errorMessage() const { return QString::fromUtf8(m_handle.strError()); }
