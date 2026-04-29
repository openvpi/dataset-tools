#pragma once

#include <QString>
#include <vector>

namespace dstools {

/// One row in transcriptions.csv
struct TranscriptionRow {
    QString name;        ///< File stem (no extension)
    QString phSeq;       ///< Space-separated phoneme sequence
    QString phDur;       ///< Space-separated phoneme durations (seconds)
    QString phNum;       ///< Space-separated word phoneme counts (optional)
    QString noteSeq;     ///< Space-separated note names (optional)
    QString noteDur;     ///< Space-separated note durations (optional)
    QString noteSlur;    ///< Space-separated slur flags (optional)
    QString noteGlide;   ///< Space-separated glide types (optional)
};

/// Unified transcriptions.csv reader/writer.
/// Format: RFC 4180 CSV with header row.
/// Handles BOM, quoted fields, mixed line endings.
class TranscriptionCsv {
public:
    /// Read from file. Column order determined by header.
    static bool read(const QString &path,
                     std::vector<TranscriptionRow> &rows,
                     QString &error);

    /// Write to file. Only writes columns that have non-empty values.
    static bool write(const QString &path,
                      const std::vector<TranscriptionRow> &rows,
                      QString &error);

    /// Parse from in-memory string (for testing).
    static bool parse(const QString &content,
                      std::vector<TranscriptionRow> &rows,
                      QString &error);
};

} // namespace dstools
