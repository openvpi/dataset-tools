#include "SliceJob.h"

#include <string>
#include <utility>
#include <vector>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <sndfile.hh>

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32)) || (defined(UNICODE) || defined(_UNICODE))
#    define USE_WIDE_CHAR
#endif

#include "AudioChunkWriter.h"
#include "AudioFileLoader.h"
#include "MarkerIO.h"
#include "Slicer.h"

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

    // --- Load audio ---
    auto loadResult = AudioFileLoader::load(filename);
    if (!loadResult.success) {
        error(loadResult.errorMessage);
        return {false, filename};
    }
    const auto &audio = loadResult.audio;
    auto totalSize = audio.totalSize();

    // --- Compute or load markers ---
    bool hasExistingMarkers = false;
    MarkerList chunks;

    if (params.loadMarkers) {
        MarkerError loadOk;
        MarkerList tmpChunks = MarkerIO::loadCSVMarkers(markerFilePath, audio.sampleRate, &loadOk);
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

        // Slicer needs a SndfileHandle; open the resolved path
#ifdef USE_WIDE_CHAR
        auto sndfilePathStr = loadResult.sndfilePath.toStdWString();
#else
        auto sndfilePathStr = loadResult.sndfilePath.toStdString();
#endif
        SndfileHandle sf(sndfilePathStr.c_str());
        Slicer slicer(&sf, params.threshold, params.minLength, params.minInterval, params.hopSize, params.maxSilKept);

        if (slicer.getErrorCode() != SlicerErrorCode::SLICER_OK) {
            error("slicer: " + slicer.getErrorMsg());
            if (!loadResult.tempFilePath.isEmpty())
                QFile::remove(loadResult.tempFilePath);
            return {false, filename};
        }

        MarkerList tmpChunks = slicer.slice();
        std::swap(chunks, tmpChunks);
    }

    // --- Save markers ---
    bool isMarkerWriteError = false;

    if (params.saveMarkers) {
        MarkerError saveOk = MarkerIO::writeCSVMarkers(chunks, markerFilePath, audio.sampleRate,
                                                       params.overwriteMarkers,
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

    // --- Write audio chunks ---
    bool isAudioWriteError = false;
    int chunksWritten = 0;

    if (params.saveAudio) {
        if (chunks.empty()) {
            error(QString("slicer: no audio chunks for output!"));
            if (!loadResult.tempFilePath.isEmpty())
                QFile::remove(loadResult.tempFilePath);
            return {false, filename};
        }

        if (!QDir().mkpath(resolvedOutPath)) {
            error(QString("filesystem: could not create directory %1.").arg(resolvedOutPath));
            if (!loadResult.tempFilePath.isEmpty())
                QFile::remove(loadResult.tempFilePath);
            return {false, filename};
        }

        auto writeResult = AudioChunkWriter::writeChunks(audio, chunks, resolvedOutPath, fileBaseName,
                                                         params.outputWaveFormat, params.minimumDigits);
        chunksWritten = writeResult.chunksWritten;
        isAudioWriteError = !writeResult.success;

        if (isAudioWriteError) {
            error(QString("%1: audio file write error (zero bytes written)").arg(filename));
        } else {
            info(QString("%1: saved %3 audio chunk(s) to %2").arg(filename, resolvedOutPath).arg(chunks.size()));
        }
    }

    // --- Cleanup ---
    if (!loadResult.tempFilePath.isEmpty())
        QFile::remove(loadResult.tempFilePath);

    if (isAudioWriteError || isMarkerWriteError) {
        return {false, filename, chunksWritten};
    }

    return {true, filename, chunksWritten};
}
