#pragma once

#include <QString>
#include <QStringView>
#include <utility>
#include <vector>

using MarkerList = std::vector<std::pair<qint64, qint64>>;

enum class MarkerTimeFormat { Decimal, Samples };

enum class MarkerError { Success, Skipped, IOError, FileNotExistError, FormatError, NothingToOutputError };

namespace MarkerIO {

    QString samplesToDecimalFormat(qint64 samples, int sampleRate);
    qint64 decimalFormatToSamples(const QStringView &decimalFormat, int sampleRate, bool *ok = nullptr);
    qint64 decimalFormatToSamples(const QString &decimalFormat, int sampleRate, bool *ok = nullptr);
    MarkerError writeCSVMarkers(const MarkerList &chunks, const QString &outFileName, int sampleRate,
                                bool overwrite = false, MarkerTimeFormat timeFormat = MarkerTimeFormat::Samples,
                                qint64 totalSize = std::numeric_limits<qint64>::max());
    MarkerList loadCSVMarkers(const QString &inFileName, int sampleRate, MarkerError *ok = nullptr);

} // namespace MarkerIO
