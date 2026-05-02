#include "MarkerIO.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

namespace {
template<class T>
inline T divIntRound(T n, T d) {
    return ((n < 0) ^ (d < 0)) ? ((n - (d / 2)) / d) : ((n + (d / 2)) / d);
}
}

namespace MarkerIO {

QString samplesToDecimalFormat(qint64 samples, int sampleRate) {
    if (sampleRate <= 0 || samples <= 0) {
        return {};
    }
    qint64 integerPart = samples / sampleRate;
    qint64 decimalPart = (samples - integerPart * sampleRate) * 1000 / sampleRate;
    qint64 secondPart = integerPart % 60;
    qint64 minutePart = integerPart / 60;
    return QString("%1:%2.%3").arg(minutePart).arg(secondPart).arg(decimalPart);
}

qint64 decimalFormatToSamples(const QStringView &decimalFormat, int sampleRate, bool *ok) {
    if (sampleRate <= 0) {
        if (ok)
            *ok = false;
        return 0;
    }
    static const QRegularExpression re(R"(^\s*(\d+):(\d+)\.(\d+)\s*$)");
    auto match = re.matchView(decimalFormat);
    if (!match.hasMatch()) {
        if (ok)
            *ok = false;
        return 0;
    }
    bool hasParseError = false;
    bool parseOk;
    qint64 mm = match.capturedView(1).toLongLong(&parseOk);
    hasParseError |= !parseOk;
    qint64 ss = match.capturedView(2).toLongLong(&parseOk);
    hasParseError |= !parseOk;
    qint64 zzz = match.capturedView(3).toLongLong(&parseOk);
    hasParseError |= !parseOk;
    if (hasParseError) {
        if (ok)
            *ok = false;
        return 0;
    }
    qint64 samples = (mm * 60 + ss) * sampleRate + divIntRound(zzz * sampleRate, static_cast<qint64>(1000));
    if (ok)
        *ok = true;
    return samples;
}

qint64 decimalFormatToSamples(const QString &decimalFormat, int sampleRate, bool *ok) {
    return decimalFormatToSamples(QStringView{decimalFormat}, sampleRate, ok);
}

MarkerError writeCSVMarkers(const MarkerList &chunks, const QString &outFileName, int sampleRate, bool overwrite,
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
        if ((frameCount <= 0) || (beginFrame > totalSize) || (endFrame > totalSize) || (beginFrame < 0) ||
            (endFrame < 0)) {
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
    auto fileError = writeFile.error();
    writeFile.close();
    return (fileError == QFile::FileError::NoError) ? MarkerError::Success : MarkerError::IOError;
}

MarkerList loadCSVMarkers(const QString &inFileName, int sampleRate, MarkerError *ok) {
    QFile readFile(inFileName);
    if (!readFile.exists()) {
        if (ok)
            *ok = MarkerError::FileNotExistError;
        return {};
    }

    if (!readFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (ok)
            *ok = MarkerError::IOError;
        return {};
    }

    MarkerList ml{};

    QTextStream ts(&readFile);
    QString line;
    bool hasReadFirstLine = false;
    while (ts.readLineInto(&line)) {
        if (!hasReadFirstLine) {
            hasReadFirstLine = true;
            continue;
        }
        auto split = QStringView{line}.split('\t');
        if (split.size() < 4) {
            if (ok)
                *ok = MarkerError::FormatError;
            return {};
        }
        auto timeFormat = split[3];
        static const QRegularExpression regexSample(R"(^\s*(\d+)\s*Hz\s*$)");
        auto regexSampleMatch = regexSample.matchView(timeFormat);
        if (regexSampleMatch.hasMatch()) {
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
                if (ok)
                    *ok = MarkerError::FormatError;
                return {};
            }

            if (parsedSampleRate != sampleRate) {
                beginFrame = divIntRound(beginFrame * sampleRate, static_cast<qint64>(parsedSampleRate));
                frameCount = divIntRound(frameCount * sampleRate, static_cast<qint64>(parsedSampleRate));
            }

            ml.emplace_back(beginFrame, beginFrame + frameCount);
        } else if (timeFormat.compare(QString("decimal"), Qt::CaseInsensitive) == 0) {
            bool hasParseError = false;
            bool parseOk;
            qint64 beginFrame = decimalFormatToSamples(split[1].toString(), sampleRate, &parseOk);
            hasParseError |= !parseOk;
            qint64 frameCount = decimalFormatToSamples(split[2].toString(), sampleRate, &parseOk);
            hasParseError |= !parseOk;

            if (hasParseError) {
                if (ok)
                    *ok = MarkerError::FormatError;
                return {};
            }
            ml.emplace_back(beginFrame, beginFrame + frameCount);
        } else {
            if (ok)
                *ok = MarkerError::FormatError;
            return {};
        }
    }
    if (ok)
        *ok = MarkerError::Success;
    return ml;
}

} // namespace MarkerIO
