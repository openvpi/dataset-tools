#include "SliceJob.h"

#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>

#include <sndfile.hh>

#include <dstools/AudioDecoder.h>

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32)) || (defined(UNICODE) || defined(_UNICODE))
#    define USE_WIDE_CHAR
#endif

#include "MarkerIO.h"
#include "MathUtils.h"
#include "Slicer.h"

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

SliceJobResult SliceJob::run(const QString &filename, const QString &outPath,
                             const SliceJobParams &params, ISliceJobSink *sink) {
    auto info = [&](const QString &msg) { if (sink) sink->onInfo(msg); };
    auto error = [&](const QString &msg) { if (sink) sink->onError(msg); };

    info(QString("%1 started processing.").arg(filename));
    qDebug() << filename;

    auto fileInfo = QFileInfo(filename);
    auto fileBaseName = fileInfo.completeBaseName();
    auto fileDirName = fileInfo.absoluteDir().absolutePath();
    auto resolvedOutPath = outPath.isEmpty() ? fileDirName : outPath;
    auto markerFilePath = QDir(fileDirName).absoluteFilePath(fileBaseName + ".csv");

#ifdef USE_WIDE_CHAR
    auto inFileNameStr = filename.toStdWString();
#else
    auto inFileNameStr = filename.toStdString();
#endif
    SndfileHandle sf(inFileNameStr.c_str());

    auto sfErrCode = sf.error();
    auto sfErrMsg = sf.strError();

    QString tempWavPath;
    if (sfErrCode) {
        dstools::audio::AudioDecoder decoder;
        if (!decoder.open(filename)) {
            error(QString("Cannot open audio file: %1 (libsndfile: %2)").arg(filename).arg(sfErrMsg));
            return {false, filename};
        }

        QTemporaryFile tempFile(QString("%1/dstslicer_XXXXXX.wav").arg(fileDirName));
        if (!tempFile.open()) {
            error(QString("Cannot create temporary file for %1").arg(filename));
            return {false, filename};
        }
        tempWavPath = tempFile.fileName();
        tempFile.close();

        const auto fmt = decoder.format();
        const int sr = fmt.sampleRate();
        const int channels = fmt.channels();
        const qint64 totalFrames = decoder.length();

        SndfileHandle wf(tempWavPath.toStdWString().c_str(), SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_FLOAT, channels, sr);
        if (wf.error()) {
            error(QString("Cannot create temporary WAV for %1").arg(filename));
            return {false, filename};
        }

        std::vector<float> buf(4096 * channels);
        qint64 framesRead = 0;
        while (framesRead < totalFrames) {
            const int toRead = std::min(static_cast<qint64>(4096), totalFrames - framesRead);
            const int n = decoder.read(buf.data(), static_cast<int>(framesRead), toRead * channels);
            if (n <= 0)
                break;
            wf.writef(buf.data(), n / channels);
            framesRead += n / channels;
        }
        decoder.close();
        wf.writeSync();

#ifdef USE_WIDE_CHAR
        sf = SndfileHandle(tempWavPath.toStdWString().c_str());
#else
        sf = SndfileHandle(tempWavPath.toStdString().c_str());
#endif
        sfErrCode = sf.error();
        if (sfErrCode) {
            error(QString("Cannot re-open decoded file: %1").arg(filename));
            return {false, filename};
        }
    }

    auto sr = sf.samplerate();
    auto channels = sf.channels();
    auto frames = sf.frames();

    auto totalSize = frames * channels;

    bool hasExistingMarkers = false;
    MarkerList chunks;

    if (params.loadMarkers) {
        MarkerError loadOk;
        MarkerList tmpChunks = MarkerIO::loadCSVMarkers(markerFilePath, sr, &loadOk);
        switch (loadOk) {
            case MarkerError::Success:
                hasExistingMarkers = true;
                info(QString("%1: loading markers from %2").arg(filename, markerFilePath));
                std::swap(chunks, tmpChunks);
                break;
            case MarkerError::FileNotExistError:
                info(QString("%1: no marker file found").arg(filename));
                break;
            case MarkerError::IOError:
                error(QString("%1: could not read marker file %2").arg(filename, markerFilePath));
                break;
            case MarkerError::FormatError:
                error(QString("%1: invalid marker file format %2").arg(filename, markerFilePath));
                break;
            default:
                break;
        }
    } else {
        hasExistingMarkers = false;
    }
    if (!hasExistingMarkers) {
        info(QString("%1: calculating markers").arg(filename));
        Slicer slicer(&sf, params.threshold, params.minLength, params.minInterval, params.hopSize, params.maxSilKept);

        if (slicer.getErrorCode() != SlicerErrorCode::SLICER_OK) {
            error("slicer: " + slicer.getErrorMsg());
            return {false, filename};
        }

        MarkerList tmpChunks = slicer.slice();
        std::swap(chunks, tmpChunks);
    }

    bool isAudioWriteError = false;
    bool isMarkerWriteError = false;
    int chunksWritten = 0;

    if (params.saveMarkers) {
        MarkerError saveOk = MarkerIO::writeCSVMarkers(chunks, markerFilePath, sr, params.overwriteMarkers,
                                                       MarkerTimeFormat::Samples, totalSize);
        switch (saveOk) {
            case MarkerError::Success:
                info(QString("%1: saved markers to %2").arg(filename, markerFilePath));
                break;
            case MarkerError::Skipped:
                info(QString("%1: marker file %2 exists, skipping").arg(filename, markerFilePath));
                break;
            case MarkerError::IOError:
                isMarkerWriteError = true;
                error(QString("%1: could not write markers to %2").arg(filename, markerFilePath));
                break;
            case MarkerError::NothingToOutputError:
                isMarkerWriteError = true;
                error(QString("%1: no markers to save").arg(filename));
                break;
            default:
                break;
        }
    }

    if (params.saveAudio) {
        if (chunks.empty()) {
            error(QString("slicer: no audio chunks for output!"));
            return {false, filename};
        }

        if (!QDir().mkpath(resolvedOutPath)) {
            error(QString("filesystem: could not create directory %1.").arg(resolvedOutPath));
            return {false, filename};
        }

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

            auto outFileName = QString("%1_%2.wav").arg(fileBaseName).arg(idx, params.minimumDigits, 10, QLatin1Char('0'));
            auto outFilePath = QDir(resolvedOutPath).absoluteFilePath(outFileName);

#ifdef USE_WIDE_CHAR
            auto outFilePathStr = outFilePath.toStdWString();
#else
            auto outFilePathStr = outFilePath.toStdString();
#endif

            int sndfile_outputWaveFormat = determineSndFileFormat(params.outputWaveFormat);
            SndfileHandle wf = SndfileHandle(outFilePathStr.c_str(), SFM_WRITE,
                                             SF_FORMAT_WAV | sndfile_outputWaveFormat, channels, sr);
            sf.seek(beginFrame, SEEK_SET);
            std::vector<double> tmp(frameCount * channels);
            sf.read(tmp.data(), tmp.size());
            auto bytesWritten = wf.write(tmp.data(), tmp.size());
            if (bytesWritten != static_cast<sf_count_t>(tmp.size())) {
                isAudioWriteError = true;
                idx++;
                break;
            }
            idx++;
        }
        chunksWritten = idx;
        if (isAudioWriteError) {
            error(QString("%1: audio file write error (zero bytes written)").arg(filename));
        } else {
            info(QString("%1: saved %3 audio chunk(s) to %2").arg(filename, resolvedOutPath).arg(chunks.size()));
        }
    }

    if (!tempWavPath.isEmpty())
        QFile::remove(tempWavPath);

    if (isAudioWriteError || isMarkerWriteError) {
        return {false, filename, chunksWritten};
    }

    return {true, filename, chunksWritten};
}
