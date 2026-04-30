/// @file TranscriptionPipeline.cpp
/// @brief Step 6→7→8 memory pipeline implementation.

#include <dstools/TranscriptionPipeline.h>
#include <dstools/TextGridToCsv.h>
#include <dstools/PhNumCalculator.h>

#include <QDir>
#include <QFileInfo>
#include <QStringList>

namespace dstools {

bool TranscriptionPipeline::run(const Options &opts,
                                GameAlignCallback gameAlign,
                                F0Callback f0Callback,
                                ProgressCallback progress,
                                QString &error) {
    std::vector<TranscriptionRow> rows;

    // Checkpoint resume: if CSV exists with MIDI columns, skip to Step 8
    if (opts.writeCsv && !opts.csvPath.isEmpty() && QFileInfo::exists(opts.csvPath)) {
        std::vector<TranscriptionRow> existingRows;
        if (TranscriptionCsv::read(opts.csvPath, existingRows, error) &&
            !existingRows.empty() && !existingRows.front().noteSeq.isEmpty()) {
            rows = std::move(existingRows);
            // Skip to Step 8
            CsvToDsConverter::Options convOpts;
            convOpts.wavsDir = opts.wavsDir;
            convOpts.outputDir = opts.outputDir;
            convOpts.sampleRate = opts.sampleRate;
            convOpts.hopSize = opts.hopSize;

            return CsvToDsConverter::convertFromMemory(rows, convOpts, f0Callback,
                [&](int cur, int tot, const QString &name) {
                    if (progress)
                        progress(Step::ConvertToDs, cur, tot, name);
                }, error);
        }
    }

    // Step 6a: TextGrid → rows
    if (!TextGridToCsv::extractDirectory(opts.textGridDir, rows, error))
        return false;

    if (progress)
        progress(Step::ExtractTextGrid, static_cast<int>(rows.size()),
                 static_cast<int>(rows.size()), {});

    // Step 6b: PhNumCalculator
    PhNumCalculator calc;
    if (!calc.loadDictionary(opts.dictPath, error))
        return false;

    if (!calc.calculateBatch(rows, error))
        return false;

    if (progress)
        progress(Step::CalculatePhNum, static_cast<int>(rows.size()),
                 static_cast<int>(rows.size()), {});

    // Write checkpoint CSV (after Step 6)
    if (opts.writeCsv && !opts.csvPath.isEmpty()) {
        if (!TranscriptionCsv::write(opts.csvPath, rows, error))
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
            if (!TranscriptionCsv::write(opts.csvPath, rows, error))
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

    return CsvToDsConverter::convertFromMemory(rows, convOpts, f0Callback,
        [&](int cur, int tot, const QString &name) {
            if (progress)
                progress(Step::ConvertToDs, cur, tot, name);
        }, error);
}

} // namespace dstools
