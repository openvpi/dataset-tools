#pragma once

/// @file TranscriptionPipeline.h
/// @brief Step 6→7→8 memory pipeline: TextGrid → CSV → DS files.

#include <dstools/TranscriptionCsv.h>
#include <dstools/CsvToDsConverter.h>
#include <functional>
#include <QString>
#include <QSet>
#include <tuple>
#include <vector>

namespace dstools {

/// @deprecated Use PipelineRunner + FormatAdapters instead. See ADR-33.
class [[deprecated("Use PipelineRunner + FormatAdapters instead")]] TranscriptionPipeline {
public:
    struct Options {
        // Input
        QString textGridDir;     ///< TextGrid files directory
        QString wavsDir;         ///< WAV files directory
        QString dictPath;        ///< Dictionary file for PhNumCalculator

        // Output
        QString outputDir;       ///< .ds files output directory

        // Options
        bool writeCsv = true;    ///< Write intermediate CSV (for checkpoint/resume)
        QString csvPath;         ///< Intermediate CSV path (required if writeCsv=true)
        int sampleRate = 44100;
        int hopSize = 512;
    };

    enum class Step {
        ExtractTextGrid = 0,  ///< Step 6a: TextGrid → TranscriptionRow
        CalculatePhNum,       ///< Step 6b: fill ph_num
        GameAlign,            ///< Step 7: GAME align → note_seq/note_dur/note_slur
        ConvertToDs,          ///< Step 8: csv2ds + RMVPE → .ds
    };

    using ProgressCallback = std::function<void(Step step, int current, int total,
                                                 const QString &name)>;

    /// GAME align callback — injected by application layer.
    /// Takes wav path + phoneme info, returns aligned notes.
    using GameAlignCallback = std::function<bool(
        const QString &wavPath,
        const std::vector<std::string> &phSeq,
        const std::vector<float> &phDur,
        const std::vector<int> &phNum,
        std::vector<std::tuple<std::string, float, int>> &notes,  // {name, dur, slur}
        QString &error)>;

    using F0Callback = CsvToDsConverter::F0Callback;

    /// @brief Injectable dependencies for testability (Issue #28).
    ///
    /// All fields default to nullptr. When nullptr, the pipeline uses the
    /// real (production) implementations. Tests can inject mocks/stubs for
    /// any subset of these to avoid file I/O and external tools.
    struct Deps {
        /// TextGrid extraction — replaces TextGridToCsv::extractDirectory.
        std::function<bool(const QString &dir,
                           std::vector<TranscriptionRow> &rows,
                           QString &error)> extractTextGrids;

        /// Dictionary loading — replaces PhNumCalculator::loadDictionary.
        /// Should populate vowels/consonants sets from any source.
        std::function<bool(const QString &dictPath,
                           QSet<QString> &vowels,
                           QSet<QString> &consonants,
                           QString &error)> loadDictionary;

        /// CSV read — replaces TranscriptionCsv::read.
        std::function<bool(const QString &path,
                           std::vector<TranscriptionRow> &rows,
                           QString &error)> readCsv;

        /// CSV write — replaces TranscriptionCsv::write.
        std::function<bool(const QString &path,
                           const std::vector<TranscriptionRow> &rows,
                           QString &error)> writeCsv;

        /// DS conversion — replaces CsvToDsConverter::convertFromMemory.
        std::function<bool(const std::vector<TranscriptionRow> &rows,
                           const CsvToDsConverter::Options &convOpts,
                           F0Callback f0Callback,
                           CsvToDsConverter::ProgressCallback progress,
                           QString &error)> convertToDs;
    };

    /// Run the full pipeline (original API — uses real implementations).
    static bool run(const Options &opts,
                    GameAlignCallback gameAlign,
                    F0Callback f0Callback,
                    ProgressCallback progress,
                    QString &error);

    /// Run the full pipeline with injected dependencies (testable overload).
    static bool run(const Options &opts,
                    GameAlignCallback gameAlign,
                    F0Callback f0Callback,
                    ProgressCallback progress,
                    const Deps &deps,
                    QString &error);

    // ── Individual step methods (for independent testing) ──────────────

    /// Step 6a: Extract TextGrid files from directory into rows.
    static bool extractTextGrids(const QString &textGridDir,
                                  std::vector<TranscriptionRow> &rows,
                                  QString &error,
                                  const Deps &deps = {});

    /// Step 6b: Calculate ph_num for each row using dictionary.
    static bool calculatePhNum(const QString &dictPath,
                                std::vector<TranscriptionRow> &rows,
                                QString &error,
                                const Deps &deps = {});

    /// Step 7: Apply GAME alignment to each row.
    static bool gameAlign(const Options &opts,
                           GameAlignCallback gameAlignCb,
                           std::vector<TranscriptionRow> &rows,
                           ProgressCallback progress,
                           QString &error);

    /// Step 8: Convert rows to .ds files.
    static bool convertToDs(const Options &opts,
                             F0Callback f0Callback,
                             const std::vector<TranscriptionRow> &rows,
                             ProgressCallback progress,
                             QString &error,
                             const Deps &deps = {});
};

} // namespace dstools
