#pragma once

#include <QString>
#include <vector>

#include <dsfw/Result.h>

namespace dstools {

/// One row in transcriptions.csv
struct TranscriptionRow {
    QString name;       ///< File stem (no extension)
    QString phSeq;      ///< Space-separated phoneme sequence
    QString phDur;      ///< Space-separated phoneme durations (seconds)
    QString phNum;      ///< Space-separated word phoneme counts (optional)
    QString noteSeq;    ///< Space-separated note names (optional)
    QString noteDur;    ///< Space-separated note durations (optional)
    QString noteSlur;   ///< Space-separated slur flags (optional)
    QString noteGlide;  ///< Space-separated glide types (optional)
    QString wordSpan;   ///< Space-separated word span indices for each phoneme
};

/// Unified transcriptions.csv reader/writer.
/// Format: RFC 4180 CSV with header row.
/// Handles BOM, quoted fields, mixed line endings.
class TranscriptionCsv {
public:
    /// Read from file. Column order determined by header.
    [[nodiscard]] static dsfw::Result<std::vector<TranscriptionRow>> read(const QString& path);

    /// Write to file. Only writes columns that have non-empty values.
    [[nodiscard]] static dsfw::Result<void> write(const QString& path, const std::vector<TranscriptionRow>& rows);

    /// Parse from in-memory string (for testing).
    [[nodiscard]] static dsfw::Result<std::vector<TranscriptionRow>> parse(const QString& content);
};

}  // namespace dstools
