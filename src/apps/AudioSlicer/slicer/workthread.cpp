#include <string>
#include <tuple>
#include <utility>
#include <vector>


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
      m_minInterval(minInterval), m_hopSize(hopSize), m_maxSilKept(maxSilKept), m_outputWaveFormat(outputWaveFormat),
      m_decoder(nullptr)
//   , m_decoderPlugin(nullptr)
{
}

void WorkThread::run() {
    emit oneInfo(QString("%1 started processing.").arg(m_filename));

    auto fileInfo = QFileInfo(m_filename);
    auto fileBaseName = fileInfo.completeBaseName();
    auto fileDirName = fileInfo.absoluteDir().absolutePath();
    auto outPath = m_outPath.isEmpty() ? fileDirName : m_outPath;

    initPlugins();
    QsMedia::WaveArguments wa;
    wa.sampleFormat = QsMedia::AV_SAMPLE_FMT_FLT;
    auto succeed = m_decoder->open(m_filename, wa);
    if (!succeed) {
        emit oneError(QString("Error opening file! ") + m_filename);
        return;
    }

    auto wavFmt = m_decoder->Format();
    int channels = wavFmt.Channels();
    int sr = wavFmt.SampleRate();
    int bits = wavFmt.BitsPerSample();
    auto frames = m_decoder->TotalSamples();

    auto total_size = frames * channels;

    Slicer slicer(m_decoder, m_threshold, m_minLength, m_minInterval, m_hopSize, m_maxSilKept);
    auto chunks = slicer.slice();

    if (chunks.empty()) {
        QString errmsg = QString("ValueError: The following conditions must be satisfied: (m_minLength >= "
                                 "m_minInterval >= m_hopSize) and (m_maxSilKept >= m_hopSize).");
        emit oneError(errmsg);
        return;
    }

    if (!QDir().mkpath(outPath)) {
        QString errmsg = QString("Could not create directory %1.").arg(outPath);
        emit oneError(errmsg);
    }

    bool isWriteError = false;
    int idx = 0;
    for (auto chunk : chunks) {
        auto beginFrame = std::get<0>(chunk);
        auto endFrame = std::get<1>(chunk);
        auto frameCount = endFrame - beginFrame;
        if ((beginFrame == endFrame) || (beginFrame > total_size) || (endFrame > total_size)) {
            continue;
        }
        qDebug() << "begin frame: " << beginFrame << " (" << 1.0 * beginFrame / sr << " seconds) " << '\n'
                 << "end frame: " << endFrame << " (" << 1.0 * endFrame / sr << " seconds) " << '\n';

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
        m_decoder->SetCurrentSample(0);
        m_decoder->SkipSamples(beginFrame * channels);
        std::vector<float> tmp(frameCount * channels);
        m_decoder->Read(tmp.data(), 0, tmp.size());
        auto bytesWritten = wf.write(tmp.data(), tmp.size());
        if (bytesWritten == 0) {
            isWriteError = true;
        }
        idx++;
    }
    if (isWriteError) {
        QString errmsg = QString("Zero bytes written");
        emit oneError(errmsg);
        return;
    }

    emit oneFinished(m_filename);
}

#include "FFmpegDecoder.h"

void WorkThread::initPlugins() {
    // #ifdef Q_OS_LINUX
    //     m_decoderLoader.setFileName("audiodecoders/libFFmpegDecoder");
    // #else
    //     m_decoderLoader.setFileName("audiodecoders/FFmpegDecoder");
    // #endif

    //     m_decoder = nullptr;

    //     m_decoderPlugin = qobject_cast<QsApi::IAudioDecoderPlugin *>(m_decoderLoader.instance());

    //     if (!m_decoderPlugin) {
    //         // QMessageBox::critical(
    //         //     this, qApp->applicationName(),
    //         //     QString("Failed to load plugins: %1!").arg(decoderLoader.errorString()));
    //         emit oneError(QString("Failed to load plugins: %1!").arg(m_decoderLoader.errorString()));
    //         goto out;
    //     }

    //     m_decoder = m_decoderPlugin->create({}, this);
    //     if (!m_decoder) {
    //         // QMessageBox::critical(this, qApp->applicationName(), QString("Failed to create decoder object!"));
    //         emit oneError(QString("Failed to create decoder object!"));
    //         goto out2;
    //     }

    m_decoder = new FFmpegDecoder();
    return;

out2:
    // Must delete before uninitializing the plugins.
    // If rely on the destructor of QWidget we would be calling unloaded code upon exit and crash the program
    delete m_decoder;

out:
    uninitPlugins();
}

void WorkThread::uninitPlugins() {
    delete m_decoder;
    // m_decoderLoader.unload();
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