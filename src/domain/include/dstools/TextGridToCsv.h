#pragma once

/// @file TextGridToCsv.h
/// @brief TextGrid → TranscriptionRow conversion for DiffSinger pipeline Step 6.

#include <dstools/TranscriptionCsv.h>
#include <QString>
#include <vector>

namespace dstools {

/// TextGrid → TranscriptionRow 转换
class TextGridToCsv {
public:
    /// Extract phoneme sequence and durations from a single TextGrid file.
    /// Reads the "phones" tier (case-insensitive), skips empty intervals.
    /// @param tgPath   Path to the .TextGrid file
    /// @param row      Output: populated with name, phSeq, phDur
    /// @param error    Set on failure
    /// @return true on success
    static bool extractFromTextGrid(const QString &tgPath,
                                     TranscriptionRow &row,
                                     QString &error);

    /// Batch-extract all .TextGrid files from a directory.
    /// Results sorted by filename (QDir::Name).
    static bool extractDirectory(const QString &dir,
                                  std::vector<TranscriptionRow> &rows,
                                  QString &error);
};

} // namespace dstools
