#pragma once

/// @file CsvToDsConverter.h
/// @brief CSV ↔ DS file conversion for DiffSinger pipeline Step 8.
/// Equivalent to Python convert_ds.py csv2ds/ds2csv.

#include <QString>
#include <dsfw/Constants.h>
#include <dsfw/Result.h>
#include <dstools/TranscriptionCsv.h>
#include <functional>
#include <vector>

namespace dstools {

    class CsvToDsConverter {
    public:
        struct Options {
            QString csvPath;
            QString wavsDir;
            QString outputDir;
            int sampleRate = constants::kDefaultSampleRate;
            int hopSize = constants::kDefaultHopSize;
            float minF0 = constants::kDefaultMinF0;
            float maxF0 = constants::kDefaultMaxF0;
        };

        using F0Callback = std::function<Result<std::vector<float>>(const QString &wavPath)>;

        using ProgressCallback = std::function<void(int current, int total, const QString &name)>;

        [[nodiscard]] static Result<void> convert(const Options &opts, F0Callback f0Callback,
                                                  ProgressCallback progress);

        [[nodiscard]] static Result<void> convertFromMemory(const std::vector<TranscriptionRow> &rows,
                                                            const Options &opts, F0Callback f0Callback,
                                                            ProgressCallback progress);

        /// Reverse: .ds directory → CSV.
        /// Two-pass: Pass 1 = .ds with .wav, Pass 2 = lone .ds (multi-sentence).
        /// Does NOT output note_slur (matching Python behavior).
        [[nodiscard]] static Result<void> dsToCsv(const QString &dsDir, const QString &csvPath);
    };

} // namespace dstools