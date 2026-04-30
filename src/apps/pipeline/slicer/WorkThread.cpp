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
#include "WorkThread.h"

inline int determineSndFileFormat(int formatEnum);

WorkThread::WorkThread(const QString &filename, const QString &outPath, double threshold, qint64 minLength,
                       qint64 minInterval, qint64 hopSize, qint64 maxSilKept, int outputWaveFormat, bool saveAudio,
                       bool saveMarkers, bool loadMarkers, bool overwriteMarkers, int minimumDigits)
    : AsyncTask(filename), m_outPath(outPath), m_threshold(threshold), m_minLength(minLength),
      m_minInterval(minInterval), m_hopSize(hopSize), m_maxSilKept(maxSilKept), m_outputWaveFormat(outputWaveFormat),
      m_saveAudio(saveAudio), m_saveMarkers(saveMarkers), m_loadMarkers(loadMarkers),
      m_overwriteMarkers(overwriteMarkers), m_minimumDigits(minimumDigits) {
}

bool WorkThread::execute(QString &msg) {
    const auto &m_filename = identifier();
    emit oneInfo(QString("%1 started processing.").arg(m_filename));
    qDebug() << m_filename;

    auto fileInfo = QFileInfo(m_filename);
    auto fileBaseName = fileInfo.completeBaseName();
    auto fileDirName = fileInfo.absoluteDir().absolutePath();
    auto outPath = m_outPath.isEmpty() ? fileDirName : m_outPath;
    auto markerFilePath = QDir(fileDirName).absoluteFilePath(fileBaseName + ".csv");

#ifdef USE_WIDE_CHAR
    auto inFileNameStr = m_filename.toStdWString();
#else
    auto inFileNameStr = m_filename.toStdString();
#endif
    SndfileHandle sf(inFileNameStr.c_str());

    auto sfErrCode = sf.error();
    auto sfErrMsg = sf.strError();

    QString tempWavPath;
    if (sfErrCode) {
        dstools::audio::AudioDecoder decoder;
        if (!decoder.open(m_filename)) {
            emit oneError(QString("Cannot open audio file: %1 (libsndfile: %2)").arg(m_filename).arg(sfErrMsg));
            msg = m_filename;
            return false;
        }

        QTemporaryFile tempFile(QString("%1/dstslicer_XXXXXX.wav").arg(fileDirName));
        if (!tempFile.open()) {
            emit oneError(QString("Cannot create temporary file for %1").arg(m_filename));
            msg = m_filename;
            return false;
        }
        tempWavPath = tempFile.fileName();
        tempFile.close();

        const auto fmt = decoder.format();
        const int sr = fmt.sampleRate();
        const int channels = fmt.channels();
        const qint64 totalFrames = decoder.length();

        SndfileHandle wf(tempWavPath.toStdWString().c_str(), SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_FLOAT, channels, sr);
        if (wf.error()) {
            emit oneError(QString("Cannot create temporary WAV for %1").arg(m_filename));
            msg = m_filename;
            return false;
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
            emit oneError(QString("Cannot re-open decoded file: %1").arg(m_filename));
            msg = m_filename;
            return false;
        }
    }

    auto sr = sf.samplerate();
    auto channels = sf.channels();
    auto frames = sf.frames();

    auto totalSize = frames * channels;

    bool hasExistingMarkers = false;
    MarkerList chunks;

    if (m_loadMarkers) {
        MarkerError loadOk;
        MarkerList tmpChunks = MarkerIO::loadCSVMarkers(markerFilePath, sr, &loadOk);
        switch (loadOk) {
            case MarkerError::Success:
                hasExistingMarkers = true;
                emit oneInfo(QString("%1: loading markers from %2").arg(m_filename, markerFilePath));
                std::swap(chunks, tmpChunks);
                break;
            case MarkerError::FileNotExistError:
                emit oneInfo(QString("%1: no marker file found").arg(m_filename));
                break;
            case MarkerError::IOError:
                emit oneError(QString("%1: could not read marker file %2").arg(m_filename, markerFilePath));
                break;
            case MarkerError::FormatError:
                emit oneError(QString("%1: invalid marker file format %2").arg(m_filename, markerFilePath));
                break;
            default:
                break;
        }
    } else {
        hasExistingMarkers = false;
    }
    if (!hasExistingMarkers) {
        emit oneInfo(QString("%1: calculating markers").arg(m_filename));
        Slicer slicer(&sf, m_threshold, m_minLength, m_minInterval, m_hopSize, m_maxSilKept);

        if (slicer.getErrorCode() != SlicerErrorCode::SLICER_OK) {
            emit oneError("slicer: " + slicer.getErrorMsg());
            msg = m_filename;
            return false;
        }

        MarkerList tmpChunks = slicer.slice();
        std::swap(chunks, tmpChunks);
    }

    bool isAudioWriteError = false;
    bool isMarkerWriteError = false;

    if (m_saveMarkers) {
        MarkerError saveOk = MarkerIO::writeCSVMarkers(chunks, markerFilePath, sr, m_overwriteMarkers,
                                                       MarkerTimeFormat::Samples, totalSize);
        switch (saveOk) {
            case MarkerError::Success:
                emit oneInfo(QString("%1: saved markers to %2").arg(m_filename, markerFilePath));
                break;
            case MarkerError::Skipped:
                emit oneInfo(QString("%1: marker file %2 exists, skipping").arg(m_filename, markerFilePath));
                break;
            case MarkerError::IOError:
                isMarkerWriteError = true;
                emit oneError(QString("%1: could not write markers to %2").arg(m_filename, markerFilePath));
                break;
            case MarkerError::NothingToOutputError:
                isMarkerWriteError = true;
                emit oneError(QString("%1: no markers to save").arg(m_filename));
                break;
            default:
                break;
        }
    }

    if (m_saveAudio) {
        if (chunks.empty()) {
            QString errmsg = QString("slicer: no audio chunks for output!");
            emit oneError(errmsg);
            msg = m_filename;
            return false;
        }

        if (!QDir().mkpath(outPath)) {
            QString errmsg = QString("filesystem: could not create directory %1.").arg(outPath);
            emit oneError(errmsg);
            return false;
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


            auto outFileName = QString("%1_%2.wav").arg(fileBaseName).arg(idx, m_minimumDigits, 10, QLatin1Char('0'));
            auto outFilePath = QDir(outPath).absoluteFilePath(outFileName);

#ifdef USE_WIDE_CHAR
            auto outFilePathStr = outFilePath.toStdWString();
#else
            auto outFilePathStr = outFilePath.toStdString();
#endif

            int sndfile_outputWaveFormat = determineSndFileFormat(m_outputWaveFormat);
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
        if (isAudioWriteError) {
            emit oneError(QString("%1: audio file write error (zero bytes written)").arg(m_filename));
        } else {
            emit oneInfo(QString("%1: saved %3 audio chunk(s) to %2").arg(m_filename, outPath).arg(chunks.size()));
        }
    }

    if (isAudioWriteError || isMarkerWriteError) {
        if (!tempWavPath.isEmpty())
            QFile::remove(tempWavPath);
        msg = m_filename;
        return false;
    }

    if (!tempWavPath.isEmpty())
        QFile::remove(tempWavPath);

    msg = m_filename;
    return true;
}

inline int determineSndFileFormat(int formatEnum) {
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
