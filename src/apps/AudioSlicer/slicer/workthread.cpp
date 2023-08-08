#include <string>
#include <tuple>
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

#include "slicer.h"
#include "workthread.h"


inline int determineSndFileFormat(int formatEnum);

WorkThread::WorkThread(QString filename, QString outPath, double threshold, qint64 minLength, qint64 minInterval,
                       qint64 hopSize, qint64 maxSilKept, int outputWaveFormat)
    : m_filename(std::move(filename)), m_outPath(std::move(outPath)), m_threshold(threshold), m_minLength(minLength),
      m_minInterval(minInterval), m_hopSize(hopSize), m_maxSilKept(maxSilKept), m_outputWaveFormat(outputWaveFormat) {}

void WorkThread::run() {
    emit oneInfo(QString("%1 started processing.").arg(m_filename));
    qDebug() << m_filename;

    auto fileInfo = QFileInfo(m_filename);
    auto fileBaseName = fileInfo.completeBaseName();
    auto fileDirName = fileInfo.absoluteDir().absolutePath();
    auto outPath = m_outPath.isEmpty() ? fileDirName : m_outPath;

#ifdef USE_WIDE_CHAR
    auto inFileNameStr = m_filename.toStdWString();
#else
    auto inFileNameStr = m_filename.toStdString();
#endif
    SndfileHandle sf(inFileNameStr.c_str());

    auto sfErrCode = sf.error();
    auto sfErrMsg = sf.strError();

    if (sfErrCode) {
        emit oneError(QString("libsndfile error %1: %2").arg(sfErrCode).arg(sfErrMsg));
        emit oneFailed(m_filename);
        return;
    }

    auto sr = sf.samplerate();
    auto channels = sf.channels();
    auto frames = sf.frames();

    auto total_size = frames * channels;

    Slicer slicer(&sf, m_threshold, m_minLength, m_minInterval, m_hopSize, m_maxSilKept);

    if (slicer.getErrorCode() != SlicerErrorCode::SLICER_OK) {
        emit oneError("slicer: " + slicer.getErrorMsg());
        emit oneFailed(m_filename);
        return;
    }

    auto chunks = slicer.slice();
    if (chunks.empty()) {
        QString errmsg = QString("slicer: no audio chunks for output!");
        emit oneError(errmsg);
        emit oneFailed(m_filename);
        return;
    }

    if (!QDir().mkpath(outPath)) {
        QString errmsg = QString("filesystem: could not create directory %1.").arg(outPath);
        emit oneError(errmsg);
        emit oneFailed(m_filename);
    }

    bool isWriteError = false;
    int idx = 0;
    for (auto chunk : chunks) {
        auto beginFrame = chunk.first;
        auto endFrame = chunk.second;
        auto frameCount = endFrame - beginFrame;
        if ((beginFrame == endFrame) || (beginFrame > total_size) || (endFrame > total_size)) {
            continue;
        }
        qDebug() << QString("  > frame: %1 -> %2, seconds: %3 -> %4")
                            .arg(beginFrame).arg(endFrame)
                            .arg(1.0 * beginFrame / sr).arg(1.0 * endFrame / sr);


        auto outFileName = QString("%1_%2.wav").arg(fileBaseName).arg(idx);
        auto outFilePath = QDir(outPath).absoluteFilePath(outFileName);

#ifdef USE_WIDE_CHAR
        auto outFilePathStr = outFilePath.toStdWString();
#else
        auto outFilePathStr = outFilePath.toStdString();
#endif

        int sndfile_outputWaveFormat = determineSndFileFormat(m_outputWaveFormat);
        SndfileHandle wf =
            SndfileHandle(outFilePathStr.c_str(), SFM_WRITE, SF_FORMAT_WAV | sndfile_outputWaveFormat, channels, sr);
        sf.seek(beginFrame, SEEK_SET);
        std::vector<double> tmp(frameCount * channels);
        auto bytesRead = sf.read(tmp.data(), tmp.size());
        auto bytesWritten = wf.write(tmp.data(), tmp.size());
        if (bytesWritten == 0) {
            isWriteError = true;
        }
        idx++;
    }
    if (isWriteError) {
        QString errmsg = QString("Zero bytes written");
        emit oneError(errmsg);
        emit oneFailed(m_filename);
        return;
    }

    emit oneFinished(m_filename);
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
    }
    return 0;
}