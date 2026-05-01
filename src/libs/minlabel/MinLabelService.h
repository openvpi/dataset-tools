#pragma once

/// @file MinLabelService.h
/// @brief MinLabel audio labeling service for G2P-converted label management.

#include <QString>
#include <QStringList>

#include <dstools/Result.h>

namespace Minlabel {

/// @brief Label file contents for a single utterance.
struct LabelData {
    QString lab;            ///< Label text (G2P-converted phoneme sequence)
    QString rawText;        ///< Original raw text before conversion
    QString labWithoutTone; ///< Tone-stripped label text
    bool isCheck = false;   ///< Review flag indicating manual check needed
};

/// @brief Result of a batch lab-to-JSON conversion operation.
struct ConvertResult {
    int count = 0;              ///< Number of successfully converted files
    QStringList failedFiles;    ///< List of files that failed to convert
};

/// @brief Static utility class for label file I/O and lab-to-JSON conversion.
class MinLabelService {
public:
    /// @brief Remove tone marks from label content.
    /// @param labContent Label string with tone marks.
    /// @return Label string with tones removed.
    static QString removeTone(const QString &labContent);

    /// @brief Load label data from a JSON file.
    /// @param jsonFilePath Path to the JSON label file.
    /// @return Label data or error.
    static dstools::Result<LabelData> loadLabel(const QString &jsonFilePath);

    /// @brief Save label data to both JSON and lab files.
    /// @param jsonFilePath Path to the output JSON file.
    /// @param labFilePath Path to the output lab file.
    /// @param data Label data to save.
    /// @return Success or error.
    static dstools::Result<void> saveLabel(const QString &jsonFilePath, const QString &labFilePath,
                                  const LabelData &data);

    /// @brief Batch convert lab files in a directory to JSON format.
    /// @param dirName Path to the directory containing lab files.
    /// @return Conversion result or error.
    static dstools::Result<ConvertResult> convertLabToJson(const QString &dirName);
};

} // namespace Minlabel
