#pragma once

/// @file TranscriptionPipeline.h
/// @brief Step 6→7→8 memory pipeline: TextGrid → CSV → DS files.

#include <dstools/TranscriptionCsv.h>
#include <dstools/CsvToDsConverter.h>
#include <functional>
#include <QString>
#include <tuple>
#include <vector>

namespace dstools {

/// Step 6→7→8 memory pipeline.
/// TextGrid directory in → .ds files out. Intermediate CSV optional.
class TranscriptionPipeline {
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

    /// Run the full pipeline.
    static bool run(const Options &opts,
                    GameAlignCallback gameAlign,
                    F0Callback f0Callback,
                    ProgressCallback progress,
                    QString &error);
};

} // namespace dstools
