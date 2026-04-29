#pragma once

/// @file CsvToDsConverter.h
/// @brief CSV ↔ DS file conversion for DiffSinger pipeline Step 8.
/// Equivalent to Python convert_ds.py csv2ds/ds2csv.

#include <dstools/TranscriptionCsv.h>
#include <functional>
#include <QString>
#include <vector>

namespace dstools {

class CsvToDsConverter {
public:
    struct Options {
        QString csvPath;
        QString wavsDir;
        QString outputDir;
        int sampleRate = 44100;
        int hopSize = 512;
    };

    /// F0 extraction callback — injected by application layer (wraps rmvpe-infer).
    /// @param wavPath WAV file path
    /// @param f0 Output: F0 values in Hz (unvoiced frames should be interpolated, not zero)
    /// @param error Error description
    using F0Callback = std::function<bool(const QString &wavPath,
                                          std::vector<float> &f0,
                                          QString &error)>;

    /// Progress callback
    using ProgressCallback = std::function<void(int current, int total,
                                                 const QString &name)>;

    /// Convert CSV file to .ds files.
    static bool convert(const Options &opts,
                        F0Callback f0Callback,
                        ProgressCallback progress,
                        QString &error);

    /// Convert from in-memory rows (for TranscriptionPipeline).
    static bool convertFromMemory(const std::vector<TranscriptionRow> &rows,
                                   const Options &opts,
                                   F0Callback f0Callback,
                                   ProgressCallback progress,
                                   QString &error);

    /// Reverse: .ds directory → CSV.
    /// Two-pass: Pass 1 = .ds with .wav, Pass 2 = lone .ds (multi-sentence).
    /// Does NOT output note_slur (matching Python behavior).
    static bool dsToCsv(const QString &dsDir,
                        const QString &csvPath,
                        QString &error);
};

} // namespace dstools
