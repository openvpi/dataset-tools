#include "AudioFileLoader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>

#include <sndfile.hh>

#include <dstools/AudioDecoder.h>

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32)) || (defined(UNICODE) || defined(_UNICODE))
#    define USE_WIDE_CHAR
#endif

AudioLoadResult AudioFileLoader::load(const QString &filePath) {
    AudioLoadResult result;

#ifdef USE_WIDE_CHAR
    auto inFileNameStr = filePath.toStdWString();
#else
    auto inFileNameStr = filePath.toStdString();
#endif
    SndfileHandle sf(inFileNameStr.c_str());

    auto sfErrCode = sf.error();
    auto sfErrMsg = sf.strError();

    if (sfErrCode) {
        dstools::audio::AudioDecoder decoder;
        if (!decoder.open(filePath)) {
            result.success = false;
            result.errorMessage =
                QString("Cannot open audio file: %1 (libsndfile: %2)").arg(filePath).arg(sfErrMsg);
            return result;
        }

        auto fileDirName = QFileInfo(filePath).absoluteDir().absolutePath();
        QTemporaryFile tempFile(QString("%1/dstslicer_XXXXXX.wav").arg(fileDirName));
        if (!tempFile.open()) {
            result.success = false;
            result.errorMessage = QString("Cannot create temporary file for %1").arg(filePath);
            return result;
        }
        result.tempFilePath = tempFile.fileName();
        tempFile.close();

        const auto fmt = decoder.format();
        const int sr = fmt.sampleRate();
        const int channels = fmt.channels();
        const qint64 totalFrames = decoder.length();

#ifdef USE_WIDE_CHAR
        auto outFileNameStr = result.tempFilePath.toStdWString();
#else
        auto outFileNameStr = result.tempFilePath.toStdString();
#endif
        SndfileHandle wf(outFileNameStr.c_str(), SFM_WRITE,
                         SF_FORMAT_WAV | SF_FORMAT_FLOAT, channels, sr);
        if (wf.error()) {
            result.success = false;
            result.errorMessage = QString("Cannot create temporary WAV for %1").arg(filePath);
            return result;
        }

        constexpr int kDefaultBufferSize = 4096;
        std::vector<float> buf(kDefaultBufferSize * channels);
        qint64 framesRead = 0;
        while (framesRead < totalFrames) {
            const int toRead = std::min(static_cast<qint64>(kDefaultBufferSize), totalFrames - framesRead);
            const int n = decoder.read(buf.data(), static_cast<int>(framesRead), toRead * channels);
            if (n <= 0)
                break;
            wf.writef(buf.data(), n / channels);
            framesRead += n / channels;
        }
        decoder.close();
        wf.writeSync();

#ifdef USE_WIDE_CHAR
        sf = SndfileHandle(result.tempFilePath.toStdWString().c_str());
#else
        sf = SndfileHandle(result.tempFilePath.toStdString().c_str());
#endif
        sfErrCode = sf.error();
        if (sfErrCode) {
            result.success = false;
            result.errorMessage = QString("Cannot re-open decoded file: %1").arg(filePath);
            return result;
        }
    }

    result.sndfilePath = result.tempFilePath.isEmpty() ? filePath : result.tempFilePath;
    result.audio.sampleRate = sf.samplerate();
    result.audio.channels = sf.channels();
    result.audio.frames = sf.frames();

    auto totalSize = result.audio.frames * result.audio.channels;
    result.audio.samples.resize(totalSize);
    sf.readf(result.audio.samples.data(), result.audio.frames);

    result.success = true;
    return result;
}
