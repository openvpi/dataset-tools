/// @file TranscriptionPipeline.cpp
/// @brief Step 6→7→8 memory pipeline implementation.

#include <dstools/TranscriptionPipeline.h>
#include <dstools/TextGridToCsv.h>
#include <dstools/PhNumCalculator.h>

#include <QDir>
#include <QFileInfo>
#include <QStringList>

namespace dstools {

// ---------------------------------------------------------------------------
// Original API — delegates to the injectable overload with empty Deps.
// ---------------------------------------------------------------------------
bool TranscriptionPipeline::run(const Options &opts,
                                GameAlignCallback gameAlign,
                                F0Callback f0Callback,
                                ProgressCallback progress,
                                QString &error) {
    return run(opts, std::move(gameAlign), std::move(f0Callback),
               std::move(progress), Deps{}, error);
}

// ---------------------------------------------------------------------------
// Injectable overload — each dependency falls back to production impl when
// the corresponding Deps callback is nullptr.
// ---------------------------------------------------------------------------
bool TranscriptionPipeline::run(const Options &opts,
                                GameAlignCallback gameAlign,
                                F0Callback f0Callback,
                                ProgressCallback progress,
                                const Deps &deps,
                                QString &error) {
    // -- Resolve dependency functions (use injected or production default) --

    auto doExtractTextGrids = deps.extractTextGrids
        ? deps.extractTextGrids
        : [](const QString &dir, std::vector<TranscriptionRow> &rows, QString &err) {
              return TextGridToCsv::extractDirectory(dir, rows, err);
          };

    auto doReadCsv = deps.readCsv
        ? deps.readCsv
        : [](const QString &path, std::vector<TranscriptionRow> &rows, QString &err) {
              return TranscriptionCsv::read(path, rows, err);
          };

    auto doWriteCsv = deps.writeCsv
        ? deps.writeCsv
        : [](const QString &path, const std::vector<TranscriptionRow> &rows, QString &err) {
              return TranscriptionCsv::write(path, rows, err);
          };

    auto doConvertToDs = deps.convertToDs
        ? deps.convertToDs
        : [](const std::vector<TranscriptionRow> &rows,
             const CsvToDsConverter::Options &convOpts,
             F0Callback f0cb,
             CsvToDsConverter::ProgressCallback prog,
             QString &err) {
              return CsvToDsConverter::convertFromMemory(rows, convOpts, std::move(f0cb),
                                                         std::move(prog), err);
          };

    std::vector<TranscriptionRow> rows;

    // Checkpoint resume: if CSV exists with MIDI columns, skip to Step 8
    if (opts.writeCsv && !opts.csvPath.isEmpty() && QFileInfo::exists(opts.csvPath)) {
        std::vector<TranscriptionRow> existingRows;
        if (doReadCsv(opts.csvPath, existingRows, error) &&
            !existingRows.empty() && !existingRows.front().noteSeq.isEmpty()) {
            rows = std::move(existingRows);
            // Skip to Step 8
            CsvToDsConverter::Options convOpts;
            convOpts.wavsDir = opts.wavsDir;
            convOpts.outputDir = opts.outputDir;
            convOpts.sampleRate = opts.sampleRate;
            convOpts.hopSize = opts.hopSize;

            return doConvertToDs(rows, convOpts, f0Callback,
                [&](int cur, int tot, const QString &name) {
                    if (progress)
                        progress(Step::ConvertToDs, cur, tot, name);
                }, error);
        }
    }

    // Step 6a: TextGrid → rows
    if (!doExtractTextGrids(opts.textGridDir, rows, error))
        return false;

    if (progress)
        progress(Step::ExtractTextGrid, static_cast<int>(rows.size()),
                 static_cast<int>(rows.size()), {});

    // Step 6b: PhNumCalculator
    PhNumCalculator calc;
    if (deps.loadDictionary) {
        QSet<QString> vowels, consonants;
        if (!deps.loadDictionary(opts.dictPath, vowels, consonants, error))
            return false;
        calc.setVowels(vowels);
        calc.setConsonants(consonants);
    } else {
        if (!calc.loadDictionary(opts.dictPath, error))
            return false;
    }

    if (!calc.calculateBatch(rows, error))
        return false;

    if (progress)
        progress(Step::CalculatePhNum, static_cast<int>(rows.size()),
                 static_cast<int>(rows.size()), {});

    // Write checkpoint CSV (after Step 6)
    if (opts.writeCsv && !opts.csvPath.isEmpty()) {
        if (!doWriteCsv(opts.csvPath, rows, error))
            return false;
    }

    // Step 7: GAME align — per row
    if (gameAlign) {
        const int total = static_cast<int>(rows.size());
        for (int i = 0; i < total; ++i) {
            auto &row = rows[i];

            // Parse row strings to vectors
            const QStringList phSeqParts = row.phSeq.split(QLatin1Char(' '), Qt::SkipEmptyParts);
            const QStringList phDurParts = row.phDur.split(QLatin1Char(' '), Qt::SkipEmptyParts);
            const QStringList phNumParts = row.phNum.split(QLatin1Char(' '), Qt::SkipEmptyParts);

            std::vector<std::string> phSeqVec;
            phSeqVec.reserve(phSeqParts.size());
            for (const auto &s : phSeqParts)
                phSeqVec.push_back(s.toStdString());

            std::vector<float> phDurVec;
            phDurVec.reserve(phDurParts.size());
            for (const auto &s : phDurParts)
                phDurVec.push_back(s.toFloat());

            std::vector<int> phNumVec;
            phNumVec.reserve(phNumParts.size());
            for (const auto &s : phNumParts)
                phNumVec.push_back(s.toInt());

            const QString wavPath = QDir(opts.wavsDir).filePath(row.name + QStringLiteral(".wav"));

            std::vector<std::tuple<std::string, float, int>> notes;
            if (!gameAlign(wavPath, phSeqVec, phDurVec, phNumVec, notes, error))
                return false;

            // Write align results back to row
            QStringList noteSeq, noteDur, noteSlur;
            noteSeq.reserve(static_cast<int>(notes.size()));
            noteDur.reserve(static_cast<int>(notes.size()));
            noteSlur.reserve(static_cast<int>(notes.size()));

            for (const auto &[name, dur, slur] : notes) {
                noteSeq << QString::fromStdString(name);
                // 6 decimal places, strip trailing zeros
                QString durStr = QString::number(dur, 'f', 6);
                // Remove trailing zeros after decimal point
                if (durStr.contains(QLatin1Char('.'))) {
                    while (durStr.endsWith(QLatin1Char('0')))
                        durStr.chop(1);
                    if (durStr.endsWith(QLatin1Char('.')))
                        durStr.chop(1);
                }
                noteDur << durStr;
                noteSlur << QString::number(slur);
            }

            row.noteSeq = noteSeq.join(QLatin1Char(' '));
            row.noteDur = noteDur.join(QLatin1Char(' '));
            row.noteSlur = noteSlur.join(QLatin1Char(' '));

            if (progress)
                progress(Step::GameAlign, i + 1, total, row.name);
        }

        // Write checkpoint CSV (after Step 7)
        if (opts.writeCsv && !opts.csvPath.isEmpty()) {
            if (!doWriteCsv(opts.csvPath, rows, error))
                return false;
        }
    }
    // If gameAlign is null, skip Step 7 (leave MIDI columns empty)

    // Step 8: CsvToDsConverter
    CsvToDsConverter::Options convOpts;
    convOpts.wavsDir = opts.wavsDir;
    convOpts.outputDir = opts.outputDir;
    convOpts.sampleRate = opts.sampleRate;
    convOpts.hopSize = opts.hopSize;

    return doConvertToDs(rows, convOpts, f0Callback,
        [&](int cur, int tot, const QString &name) {
            if (progress)
                progress(Step::ConvertToDs, cur, tot, name);
        }, error);
}

// ---------------------------------------------------------------------------
// Individual step methods — for independent testing
// ---------------------------------------------------------------------------

bool TranscriptionPipeline::extractTextGrids(const QString &textGridDir,
                                              std::vector<TranscriptionRow> &rows,
                                              QString &error,
                                              const Deps &deps) {
    if (deps.extractTextGrids)
        return deps.extractTextGrids(textGridDir, rows, error);
    return TextGridToCsv::extractDirectory(textGridDir, rows, error);
}

bool TranscriptionPipeline::calculatePhNum(const QString &dictPath,
                                            std::vector<TranscriptionRow> &rows,
                                            QString &error,
                                            const Deps &deps) {
    PhNumCalculator calc;
    if (deps.loadDictionary) {
        QSet<QString> vowels, consonants;
        if (!deps.loadDictionary(dictPath, vowels, consonants, error))
            return false;
        calc.setVowels(vowels);
        calc.setConsonants(consonants);
    } else {
        if (!calc.loadDictionary(dictPath, error))
            return false;
    }
    return calc.calculateBatch(rows, error);
}

bool TranscriptionPipeline::gameAlign(const Options &opts,
                                       GameAlignCallback gameAlignCb,
                                       std::vector<TranscriptionRow> &rows,
                                       ProgressCallback progress,
                                       QString &error) {
    if (!gameAlignCb) {
        error = "GAME align callback is null";
        return false;
    }

    const int total = static_cast<int>(rows.size());
    for (int i = 0; i < total; ++i) {
        auto &row = rows[i];

        const QStringList phSeqParts = row.phSeq.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        const QStringList phDurParts = row.phDur.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        const QStringList phNumParts = row.phNum.split(QLatin1Char(' '), Qt::SkipEmptyParts);

        std::vector<std::string> phSeqVec;
        phSeqVec.reserve(phSeqParts.size());
        for (const auto &s : phSeqParts)
            phSeqVec.push_back(s.toStdString());

        std::vector<float> phDurVec;
        phDurVec.reserve(phDurParts.size());
        for (const auto &s : phDurParts)
            phDurVec.push_back(s.toFloat());

        std::vector<int> phNumVec;
        phNumVec.reserve(phNumParts.size());
        for (const auto &s : phNumParts)
            phNumVec.push_back(s.toInt());

        const QString wavPath = QDir(opts.wavsDir).filePath(row.name + QStringLiteral(".wav"));

        std::vector<std::tuple<std::string, float, int>> notes;
        if (!gameAlignCb(wavPath, phSeqVec, phDurVec, phNumVec, notes, error))
            return false;

        QStringList noteSeq, noteDur, noteSlur;
        noteSeq.reserve(static_cast<int>(notes.size()));
        noteDur.reserve(static_cast<int>(notes.size()));
        noteSlur.reserve(static_cast<int>(notes.size()));

        for (const auto &[name, dur, slur] : notes) {
            noteSeq << QString::fromStdString(name);
            QString durStr = QString::number(dur, 'f', 6);
            if (durStr.contains(QLatin1Char('.'))) {
                while (durStr.endsWith(QLatin1Char('0')))
                    durStr.chop(1);
                if (durStr.endsWith(QLatin1Char('.')))
                    durStr.chop(1);
            }
            noteDur << durStr;
            noteSlur << QString::number(slur);
        }

        row.noteSeq = noteSeq.join(QLatin1Char(' '));
        row.noteDur = noteDur.join(QLatin1Char(' '));
        row.noteSlur = noteSlur.join(QLatin1Char(' '));

        if (progress)
            progress(Step::GameAlign, i + 1, total, row.name);
    }
    return true;
}

bool TranscriptionPipeline::convertToDs(const Options &opts,
                                         F0Callback f0Callback,
                                         const std::vector<TranscriptionRow> &rows,
                                         ProgressCallback progress,
                                         QString &error,
                                         const Deps &deps) {
    CsvToDsConverter::Options convOpts;
    convOpts.wavsDir = opts.wavsDir;
    convOpts.outputDir = opts.outputDir;
    convOpts.sampleRate = opts.sampleRate;
    convOpts.hopSize = opts.hopSize;

    if (deps.convertToDs) {
        return deps.convertToDs(rows, convOpts, f0Callback,
            [&](int cur, int tot, const QString &name) {
                if (progress)
                    progress(Step::ConvertToDs, cur, tot, name);
            }, error);
    }

    return CsvToDsConverter::convertFromMemory(rows, convOpts, f0Callback,
        [&](int cur, int tot, const QString &name) {
            if (progress)
                progress(Step::ConvertToDs, cur, tot, name);
        }, error);
}

} // namespace dstools
