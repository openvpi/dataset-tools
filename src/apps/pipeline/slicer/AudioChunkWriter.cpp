#include "AudioChunkWriter.h"

#include <QDebug>
#include <QDir>

#include <sndfile.hh>

#include "AudioFileLoader.h"
#include "Enumerations.h"

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32)) || (defined(UNICODE) || defined(_UNICODE))
#    define USE_WIDE_CHAR
#endif

static int determineSndFileFormat(int formatEnum) {
    switch (formatEnum) {
        case WF_INT16_PCM:
            return SF_FORMAT_PCM_16;
        case WF_INT24_PCM:
            return SF_FORMAT_PCM_24;
        case WF_INT32_PCM:
            return SF_FORMAT_PCM_32;
        case WF_FLOAT32:
            return SF_FORMAT_FLOAT;
        default:
            return 0;
    }
}

ChunkWriteResult AudioChunkWriter::writeChunks(const AudioData &audio, const MarkerList &chunks,
                                                const QString &outputDir, const QString &fileBaseName,
                                                int outputWaveFormat, int minimumDigits) {
    ChunkWriteResult result;
    auto totalSize = audio.totalSize();
    auto sr = audio.sampleRate;
    auto channels = audio.channels;

    int idx = 0;
    for (auto chunk : chunks) {
        auto beginFrame = chunk.first;
        auto endFrame = chunk.second;
        auto frameCount = endFrame - beginFrame;
        if ((frameCount <= 0) || (beginFrame > totalSize) || (endFrame > totalSize) || (beginFrame < 0) ||
            (endFrame < 0)) {
            continue;
        }
        qDebug() << QString("  > frame: %1 -> %2, seconds: %3 -> %4")
                        .arg(beginFrame)
                        .arg(endFrame)
                        .arg(1.0 * beginFrame / sr)
                        .arg(1.0 * endFrame / sr);

        auto outFileName =
            QString("%1_%2.wav").arg(fileBaseName).arg(idx, minimumDigits, 10, QLatin1Char('0'));
        auto outFilePath = QDir(outputDir).absoluteFilePath(outFileName);

#ifdef USE_WIDE_CHAR
        auto outFilePathStr = outFilePath.toStdWString();
#else
        auto outFilePathStr = outFilePath.toStdString();
#endif

        int sndfile_outputWaveFormat = determineSndFileFormat(outputWaveFormat);
        SndfileHandle wf = SndfileHandle(outFilePathStr.c_str(), SFM_WRITE,
                                         SF_FORMAT_WAV | sndfile_outputWaveFormat, channels, sr);
        if (!wf) {
            result.success = false;
            result.errorMessage = QString("Failed to open output file for writing: %1").arg(outFilePath);
            result.chunksWritten = idx + 1;
            return result;
        }

        // Write chunk from the pre-loaded sample buffer
        auto sampleOffset = beginFrame * channels;
        auto sampleCount = frameCount * channels;
        auto bytesWritten = wf.write(audio.samples.data() + sampleOffset, sampleCount);
        if (bytesWritten != static_cast<sf_count_t>(sampleCount)) {
            result.success = false;
            result.chunksWritten = idx + 1;
            return result;
        }
        idx++;
    }

    result.success = true;
    result.chunksWritten = idx;
    return result;
}
