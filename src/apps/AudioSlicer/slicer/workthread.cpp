#include <string>
#include <tuple>
#include <cmath>
#include <utility>
#include <vector>
#include <limits>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStringView>
#include <QRegularExpression>
#include <QTime>

#include <sndfile.hh>

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32)) || (defined(UNICODE) || defined(_UNICODE))
#    define USE_WIDE_CHAR
#endif

#include "mathutils.h"
#include "slicer.h"
#include "workthread.h"

enum class MarkerTimeFormat {
    Decimal,
    Samples
};

enum class MarkerError {
    Success,
    Skipped,
    IOError,
    FileNotExistError,
    FormatError,
    NothingToOutputError
};

inline int determineSndFileFormat(int formatEnum);
inline MarkerError writeCSVMarkers(const MarkerList& chunks, const QString &outFileName, int sampleRate,
                                   bool overwrite = false,
                                   MarkerTimeFormat timeFormat = MarkerTimeFormat::Samples,
                                   qint64 totalSize = std::numeric_limits<qint64>::max());
inline MarkerList loadCSVMarkers(const QString &inFileName, int sampleRate, MarkerError *ok = nullptr);
inline QString samplesToDecimalFormat(qint64 samples, int sampleRate);
inline qint64 decimalFormatToSamples(const QStringView &decimalFormat, int sampleRate, bool *ok = nullptr);
inline qint64 decimalFormatToSamples(const QString &decimalFormat, int sampleRate, bool *ok = nullptr);

WorkThread::WorkThread(const QString &filename, const QString &outPath, double threshold, qint64 minLength, qint64 minInterval,
                       qint64 hopSize, qint64 maxSilKept, int outputWaveFormat,
                       bool saveAudio, bool saveMarkers, bool loadMarkers, bool overwriteMarkers,
                       int minimumDigits, int listIndex)
    : m_filename(filename), m_outPath(outPath), m_threshold(threshold), m_minLength(minLength),
      m_minInterval(minInterval), m_hopSize(hopSize), m_maxSilKept(maxSilKept), m_outputWaveFormat(outputWaveFormat),
      m_saveAudio(saveAudio), m_saveMarkers(saveMarkers), m_loadMarkers(loadMarkers), m_overwriteMarkers(overwriteMarkers),
      m_minimumDigits(minimumDigits), m_listIndex(listIndex) {}

void WorkThread::run() {
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

    if (sfErrCode) {
        emit oneError(QString("libsndfile error %1: %2").arg(sfErrCode).arg(sfErrMsg));
        emit oneFailed(m_filename, m_listIndex);
        return;
    }

    auto sr = sf.samplerate();
    auto channels = sf.channels();
    auto frames = sf.frames();

    auto totalSize = frames * channels;

    bool hasExistingMarkers = false;
    MarkerList chunks;

    if (m_loadMarkers) {
        MarkerError loadOk;
        MarkerList tmpChunks = loadCSVMarkers(markerFilePath, sr, &loadOk);
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
            emit oneFailed(m_filename, m_listIndex);
            return;
        }

        MarkerList tmpChunks = slicer.slice();
        std::swap(chunks, tmpChunks);
    }

    bool isAudioWriteError = false;
    bool isMarkerWriteError = false;

    if (m_saveMarkers) {
        MarkerError saveOk = writeCSVMarkers(chunks, markerFilePath, sr, m_overwriteMarkers, MarkerTimeFormat::Samples, totalSize);
        switch (saveOk) {
            case MarkerError::Success:
                oneInfo(QString("%1: saved markers to %2").arg(m_filename, markerFilePath));
                break;
            case MarkerError::Skipped:
                oneInfo(QString("%1: marker file %2 exists, skipping").arg(m_filename, markerFilePath));
                break;
            case MarkerError::IOError:
                isMarkerWriteError = true;
                oneError(QString("%1: could not write markers to %2").arg(m_filename, markerFilePath));
                break;
            case MarkerError::NothingToOutputError:
                isMarkerWriteError = true;
                oneError(QString("%1: no markers to save").arg(m_filename));
                break;
            default:
                break;
        }
    }

    if (m_saveAudio) {
        if (chunks.empty()) {
            QString errmsg = QString("slicer: no audio chunks for output!");
            emit oneError(errmsg);
            emit oneFailed(m_filename, m_listIndex);
            return;
        }

        if (!QDir().mkpath(outPath)) {
            QString errmsg = QString("filesystem: could not create directory %1.").arg(outPath);
            emit oneError(errmsg);
            emit oneFailed(m_filename, m_listIndex);
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
            auto bytesRead = sf.read(tmp.data(), tmp.size());
            auto bytesWritten = wf.write(tmp.data(), tmp.size());
            if (bytesWritten == 0) {
                isAudioWriteError = true;
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
        emit oneFailed(m_filename, m_listIndex);
        return;
    }

    emit oneFinished(m_filename, m_listIndex);
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

inline QString samplesToDecimalFormat(qint64 samples, int sampleRate) {
    if (sampleRate <= 0 || samples <= 0) {
        return "";
    }
    qint64 integerPart = samples / sampleRate;
    qint64 decimalPart = (samples - integerPart * sampleRate) * 1000 / sampleRate;
    qint64 secondPart = integerPart % 60;
    qint64 minutePart = integerPart / 60;
    return QString("%1:%2.%3").arg(minutePart).arg(secondPart).arg(decimalPart);
}

inline qint64 decimalFormatToSamples(const QStringView &decimalFormat, int sampleRate, bool *ok) {
    if (sampleRate <= 0) {
        if (ok) *ok = false;
        return 0;
    }
    QRegularExpression re(R"(^\s*(\d+):(\d+)\.(\d+)\s*$)");
    auto match = re.match(decimalFormat);
    if (!match.hasMatch()) {
        if (ok) *ok = false;
        return 0;
    }
    bool hasParseError = false;
    bool parseOk;
    qint64 mm = match.capturedView(0).toLongLong(&parseOk);
    hasParseError |= !parseOk;
    qint64 ss = match.capturedView(1).toLongLong(&parseOk);
    hasParseError |= !parseOk;
    qint64 zzz = match.capturedView(2).toLongLong(&parseOk);
    hasParseError |= !parseOk;
    if (hasParseError) {
        if (ok) *ok = false;
        return 0;
    }
    qint64 samples = (mm * 60 + ss) * sampleRate + divIntRound(zzz * sampleRate, static_cast<qint64>(1000));
    if (ok) *ok = true;
    return samples;
}

inline qint64 decimalFormatToSamples(const QString &decimalFormat, int sampleRate, bool *ok) {
    return decimalFormatToSamples(QStringView{decimalFormat}, sampleRate, ok);
}

inline MarkerError writeCSVMarkers(const MarkerList& chunks, const QString &outFileName, int sampleRate,
                            bool overwrite,
                            MarkerTimeFormat timeFormat, qint64 totalSize) {
    QString markerType = "Cue";
    if (chunks.empty()) {
        return MarkerError::NothingToOutputError;
    }
    QFile writeFile(outFileName);
    if (writeFile.exists() && (!overwrite)) {
        return MarkerError::Skipped;
    }
    if (!writeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return MarkerError::IOError;
    }
    QTextStream ts(&writeFile);
    ts << "Name\tStart\tDuration\tTime Format\tType\tDescription\n";
    int idx = 0;
    for (const auto &chunk : chunks) {
        auto beginFrame = chunk.first;
        auto endFrame = chunk.second;
        auto frameCount = endFrame - beginFrame;
        if ((frameCount <= 0) || (beginFrame > totalSize) || (endFrame > totalSize) ||
            (beginFrame < 0) || (endFrame < 0)) {
            continue;
        }
        switch (timeFormat) {
            case MarkerTimeFormat::Samples:
                ts << QString("Marker %1\t%2\t%3\t%4 Hz\t%5\t\n")
                          .arg(idx)
                          .arg(beginFrame)
                          .arg(frameCount)
                          .arg(sampleRate)
                          .arg(markerType);
                break;
            case MarkerTimeFormat::Decimal:
                ts << QString("Marker %1\t%2\t%3\tdecimal\t%4\t\n")
                          .arg(idx)
                          .arg(samplesToDecimalFormat(beginFrame, sampleRate),
                               samplesToDecimalFormat(frameCount, sampleRate))
                          .arg(markerType);
                break;
        }
        idx++;
    }
    writeFile.close();
    return (writeFile.error() == QFile::FileError::NoError) ? \
            MarkerError::Success : MarkerError::IOError;
}

inline MarkerList loadCSVMarkers(const QString &inFileName, int sampleRate, MarkerError *ok) {
    QFile readFile(inFileName);
    if (!readFile.exists()) {
        if (ok) *ok = MarkerError::FileNotExistError;
        return {};
    }

    if (!readFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (ok) *ok = MarkerError::IOError;
        return {};
    }

    MarkerList ml {};

    QTextStream ts(&readFile);
    QString line;
    bool hasReadFirstLine = false;
    while (ts.readLineInto(&line)) {
        if (!hasReadFirstLine) {
            // Skip header line
            hasReadFirstLine = true;
            continue;
        }
        auto split = QStringView{line}.split('\t');
        // [0]Name [1]Start [2]Duration [3]Time Format [4]Type [5]Description
        // We only need "Start", "Duration", and "Time Format". Read them by position.
        if (split.size() < 4) {
            if (ok) *ok = MarkerError::FormatError;
            return {};
        }
        // Identify Time Format
        auto timeFormat = split[3];
        QRegularExpression regexSample(R"(^\s*(\d+)\s*Hz\s*$)");
        auto regexSampleMatch = regexSample.match(timeFormat);
        if (regexSampleMatch.hasMatch()) {
            // MarkerTimeFormat::Samples
            bool hasParseError = false;
            bool parseOk;

            int parsedSampleRate = regexSampleMatch.capturedView(1).toInt(&parseOk);
            hasParseError |= !parseOk;
            hasParseError |= (parsedSampleRate <= 0);

            qint64 beginFrame = split[1].toLongLong(&parseOk);
            hasParseError |= !parseOk;

            qint64 frameCount = split[2].toLongLong(&parseOk);
            hasParseError |= !parseOk;

            if (hasParseError) {
                if (ok) *ok = MarkerError::FormatError;
                return {};
            }

            if (parsedSampleRate != sampleRate) {
                beginFrame = divIntRound(beginFrame * sampleRate, static_cast<qint64>(parsedSampleRate));
                frameCount = divIntRound(frameCount * sampleRate, static_cast<qint64>(parsedSampleRate));
            }

            ml.emplace_back(beginFrame, beginFrame + frameCount);
        } else if (timeFormat.compare(QString("decimal"), Qt::CaseInsensitive) == 0) {
            // MarkerTimeFormat::Decimal
            bool hasParseError = false;
            bool parseOk;
            qint64 beginFrame = decimalFormatToSamples(split[1].toString(), sampleRate, &parseOk);
            hasParseError |= !parseOk;
            qint64 frameCount = decimalFormatToSamples(split[2].toString(), sampleRate, &parseOk);
            hasParseError |= !parseOk;

            if (hasParseError) {
                if (ok) *ok = MarkerError::FormatError;
                return {};
            }
            ml.emplace_back(beginFrame, beginFrame + frameCount);
        } else {
            if (ok) *ok = MarkerError::FormatError;
            return {};
        }
    }
    if (ok) *ok = MarkerError::Success;
    return ml;
}