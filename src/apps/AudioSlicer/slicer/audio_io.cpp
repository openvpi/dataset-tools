#include "audio_io_sndfile_p.h"

using AudioReader = SndfileReader;
using AudioWriter = SndfileWriter;

std::unique_ptr<IAudioReader> createAudioReader(const QString &filePath) {
    return std::make_unique<AudioReader>(filePath);
}

std::unique_ptr<IAudioWriter> createWavWriter(
    const QString &filePath, int sampleRate, int channels,
    OutputSampleFormat format)
{
    return std::make_unique<AudioWriter>(filePath, sampleRate, channels, format);
}
