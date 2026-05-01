/// @file MarkerIO.h
/// @brief CSV marker I/O for audio slice boundaries.

#pragma once

#include <QString>
#include <QStringView>
#include <utility>
#include <vector>

using MarkerList = std::vector<std::pair<qint64, qint64>>;

/// @brief Time format for marker CSV files.
enum class MarkerTimeFormat {
    Decimal, ///< Decimal seconds format (e.g., "1.500").
    Samples  ///< Raw sample count format.
};

/// @brief Error codes for marker I/O operations.
enum class MarkerError {
    Success,            ///< Operation completed successfully.
    Skipped,            ///< Operation was skipped (e.g., file exists and no overwrite).
    IOError,            ///< General I/O error.
    FileNotExistError,  ///< Input file does not exist.
    FormatError,        ///< CSV format is invalid.
    NothingToOutputError ///< No markers to write.
};

/// @brief Functions for reading and writing audio slice marker CSV files.
namespace MarkerIO {

    /// @brief Convert a sample count to decimal time string.
    /// @param samples Sample count.
    /// @param sampleRate Sample rate in Hz.
    /// @return Formatted decimal time string.
    QString samplesToDecimalFormat(qint64 samples, int sampleRate);

    /// @brief Convert a decimal time string to sample count.
    /// @param decimalFormat Decimal time string.
    /// @param sampleRate Sample rate in Hz.
    /// @param ok Optional success flag.
    /// @return Sample count.
    qint64 decimalFormatToSamples(const QStringView &decimalFormat, int sampleRate, bool *ok = nullptr);

    /// @overload
    qint64 decimalFormatToSamples(const QString &decimalFormat, int sampleRate, bool *ok = nullptr);

    /// @brief Write slice markers to a CSV file.
    /// @param chunks List of (start, end) sample pairs.
    /// @param outFileName Output CSV file path.
    /// @param sampleRate Sample rate in Hz.
    /// @param overwrite Whether to overwrite an existing file.
    /// @param timeFormat Time format for the output.
    /// @param totalSize Total audio size for clamping end markers.
    /// @return MarkerError status code.
    MarkerError writeCSVMarkers(const MarkerList &chunks, const QString &outFileName, int sampleRate,
                                bool overwrite = false, MarkerTimeFormat timeFormat = MarkerTimeFormat::Samples,
                                qint64 totalSize = std::numeric_limits<qint64>::max());

    /// @brief Load slice markers from a CSV file.
    /// @param inFileName Input CSV file path.
    /// @param sampleRate Sample rate in Hz for decimal format conversion.
    /// @param ok Optional error status output.
    /// @return List of (start, end) sample pairs.
    MarkerList loadCSVMarkers(const QString &inFileName, int sampleRate, MarkerError *ok = nullptr);

} // namespace MarkerIO
