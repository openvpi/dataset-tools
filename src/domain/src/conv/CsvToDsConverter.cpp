#include "sndfile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <algorithm>
#include <cmath>
#include <audio-util/Util.h>
#include <dsfw/PathUtils.h>
#include <dstools/CsvAdapter.h>
#include <dstools/CsvToDsConverter.h>
#include <dstools/DsDocument.h>
#include <dstools/StringUtils.h>
#include <iomanip>
#include <locale>
#include <sstream>

namespace dstools {

    namespace {

        QString reroundDurs(const QString &durStr) {
            if (durStr.isEmpty())
                return durStr;
            const QStringList parts = durStr.split(QLatin1Char(' '), Qt::SkipEmptyParts);
            QStringList result;
            result.reserve(parts.size());
            for (const QString &p : parts) {
                result.append(dsfw::formatDur6(p.toDouble()));
            }
            return result.join(QLatin1Char(' '));
        }

        QString formatF0Seq(const std::vector<float> &f0) {
            std::ostringstream oss;
            oss.imbue(std::locale::classic());
            oss << std::fixed << std::setprecision(1);
            for (size_t i = 0; i < f0.size(); ++i) {
                if (i > 0)
                    oss << ' ';
                oss << f0[i];
            }
            return QString::fromStdString(oss.str());
        }

        Result<std::vector<float>> extractF0Builtin(const QString &wavPath, int sampleRate, int hopSize,
                                                    float minF0, float maxF0) {
            std::vector<float> mono;
            int actualSampleRate;
            std::string msg;

            if (!AudioUtil::readAudioToMonoFloat(dsfw::PathUtils::toStdPath(wavPath), mono, actualSampleRate, msg)) {
                return Result<std::vector<float>>::Error(msg);
            }

            const sf_count_t totalFrames = static_cast<sf_count_t>(mono.size());

            const int minLag = sampleRate / static_cast<int>(maxF0);
            const int maxLag = sampleRate / static_cast<int>(minF0);
            const int windowSize = maxLag * 2;
            const int numFrames = static_cast<int>((totalFrames - windowSize) / hopSize) + 1;

            std::vector<float> f0(std::max(numFrames, 0), 0.0f);

            for (int frame = 0; frame < numFrames; ++frame) {
                const int offset = frame * hopSize;
                if (offset + windowSize > static_cast<int>(totalFrames))
                    break;

                const float *win = mono.data() + offset;

                float energy = 0.0f;
                for (int i = 0; i < windowSize; ++i)
                    energy += win[i] * win[i];

                if (energy < 1e-6f * windowSize) {
                    f0[static_cast<size_t>(frame)] = 0.0f;
                    continue;
                }

                float bestCorr = -1.0f;
                int bestLag = 0;

                for (int lag = minLag; lag <= maxLag; ++lag) {
                    float corr = 0.0f;
                    float norm1 = 0.0f;
                    float norm2 = 0.0f;
                    const int len = windowSize - lag;
                    for (int i = 0; i < len; ++i) {
                        corr += win[i] * win[i + lag];
                        norm1 += win[i] * win[i];
                        norm2 += win[i + lag] * win[i + lag];
                    }
                    const float denom = std::sqrt(norm1 * norm2);
                    if (denom > 1e-10f)
                        corr /= denom;
                    else
                        corr = 0.0f;

                    if (corr > bestCorr) {
                        bestCorr = corr;
                        bestLag = lag;
                    }
                }

                if (bestCorr > 0.5f && bestLag > 0) {
                    f0[static_cast<size_t>(frame)] =
                        static_cast<float>(sampleRate) / static_cast<float>(bestLag);
                } else {
                    f0[static_cast<size_t>(frame)] = 0.0f;
                }
            }

            return f0;
        }

    } // anonymous namespace

    Result<void> CsvToDsConverter::convert(const Options &opts, F0Callback f0Callback, ProgressCallback progress) {
        std::vector<TranscriptionRow> rows;
        auto result = CsvAdapter::readRows(opts.csvPath, rows);
        if (!result.ok()) {
            return Result<void>::Error(result.error());
        }

        return convertFromMemory(rows, opts, f0Callback, progress);
    }

    Result<void> CsvToDsConverter::convertFromMemory(const std::vector<TranscriptionRow> &rows, const Options &opts,
                                                     F0Callback f0Callback, ProgressCallback progress) {
        QDir outDir(opts.outputDir);
        if (!outDir.exists()) {
            outDir.mkpath(QStringLiteral("."));
        }

        const int total = static_cast<int>(rows.size());

        for (int i = 0; i < total; ++i) {
            const auto &row = rows[i];

            if (progress)
                progress(i + 1, total, row.name);

            const QString wavPath = dsfw::PathUtils::fromStdPath(
                dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(opts.wavsDir),
                                      (row.name + QStringLiteral(".wav")).toStdString()));
            const QString dsPath = dsfw::PathUtils::fromStdPath(
                dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(opts.outputDir),
                                      (row.name + QStringLiteral(".ds")).toStdString()));

            auto f0Result = f0Callback
                ? f0Callback(wavPath)
                : extractF0Builtin(wavPath, opts.sampleRate, opts.hopSize, opts.minF0, opts.maxF0);
            if (!f0Result.ok()) {
                return Result<void>::Error(f0Result.error());
            }
            const auto &f0 = f0Result.value();

            nlohmann::json sentence;
            sentence["offset"] = 0.0;

            sentence["text"] = row.phSeq.toStdString();
            sentence["ph_seq"] = row.phSeq.toStdString();

            sentence["ph_dur"] = reroundDurs(row.phDur).toStdString();

            if (!row.phNum.isEmpty())
                sentence["ph_num"] = row.phNum.toStdString();

            if (!row.noteSeq.isEmpty())
                sentence["note_seq"] = row.noteSeq.toStdString();

            if (!row.noteDur.isEmpty())
                sentence["note_dur"] = reroundDurs(row.noteDur).toStdString();

            if (!row.noteSlur.isEmpty())
                sentence["note_slur"] = row.noteSlur.toStdString();

            if (!row.noteGlide.isEmpty())
                sentence["note_glide"] = row.noteGlide.toStdString();

            sentence["f0_seq"] = formatF0Seq(f0).toStdString();

            sentence["f0_timestep"] = std::to_string(static_cast<double>(opts.hopSize) / opts.sampleRate);

            DsDocument doc;
            doc.addRawSentence(sentence.dump());
            auto saveResult = doc.saveFile(dsPath);
            if (!saveResult) {
                return Result<void>::Error(saveResult.error());
            }
        }

        return Result<void>::Ok();
    }

    Result<void> CsvToDsConverter::dsToCsv(const QString &dsDir, const QString &csvPath) {
        QDir dir(dsDir);
        if (!dir.exists()) {
            return Result<void>::Error(
                (QStringLiteral("Directory does not exist: ") + dsDir).toStdString());
        }
        const QFileInfoList entries = dir.entryInfoList({QStringLiteral("*.ds")}, QDir::Files, QDir::Name);

        std::vector<TranscriptionRow> rows;
        bool anyWithGlide = false;

        for (const QFileInfo &entry : entries) {
            const QString wavPath = dsfw::PathUtils::fromStdPath(
                dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(entry.absolutePath()),
                                      (entry.completeBaseName() + QStringLiteral(".wav")).toStdString()));
            if (!QFile::exists(wavPath))
                continue;

            auto docResult = DsDocument::loadFile(entry.absoluteFilePath());
            if (!docResult) {
                return Result<void>::Error(docResult.error());
            }
            DsDocument doc = std::move(*docResult);
            if (doc.isEmpty())
                return Result<void>::Error("Empty document: " + entry.fileName().toStdString());

            const auto sv = doc.sentenceView(0);

            TranscriptionRow row;
            row.name = entry.completeBaseName();
            row.phSeq = sv.phSeq;
            row.phDur = reroundDurs(sv.phDur);
            row.phNum = sv.phNum;
            row.noteSeq = sv.noteSeq;
            row.noteDur = reroundDurs(sv.noteDur);

            if (!sv.noteGlide.isEmpty()) {
                anyWithGlide = true;
                row.noteGlide = sv.noteGlide;
            }

            rows.push_back(std::move(row));
        }

        for (const QFileInfo &entry : entries) {
            const QString wavPath = dsfw::PathUtils::fromStdPath(
                dsfw::PathUtils::join(dsfw::PathUtils::toStdPath(entry.absolutePath()),
                                      (entry.completeBaseName() + QStringLiteral(".wav")).toStdString()));
            if (QFile::exists(wavPath))
                continue;

            auto docResult = DsDocument::loadFile(entry.absoluteFilePath());
            if (!docResult) {
                return Result<void>::Error(docResult.error());
            }
            DsDocument doc = std::move(*docResult);
            if (doc.isEmpty())
                return Result<void>::Error("Empty document: " + entry.fileName().toStdString());

            const int count = doc.sentenceCount();
            for (int idx = 0; idx < count; ++idx) {
                const auto sv = doc.sentenceView(idx);

                TranscriptionRow row;
                if (count > 1) {
                    row.name = entry.completeBaseName() + QStringLiteral("#") + QString::number(idx);
                } else {
                    row.name = entry.completeBaseName();
                }

                row.phSeq = sv.phSeq;
                row.phDur = reroundDurs(sv.phDur);
                row.phNum = sv.phNum;
                row.noteSeq = sv.noteSeq;
                row.noteDur = reroundDurs(sv.noteDur);

                if (!sv.noteGlide.isEmpty()) {
                    anyWithGlide = true;
                    row.noteGlide = sv.noteGlide;
                }

                rows.push_back(std::move(row));
            }
        }

        if (anyWithGlide) {
            for (auto &row : rows) {
                if (row.noteGlide.isEmpty() && !row.noteSeq.isEmpty()) {
                    const int noteCount = row.noteSeq.split(QLatin1Char(' '), Qt::SkipEmptyParts).size();
                    QStringList nones;
                    nones.reserve(noteCount);
                    for (int n = 0; n < noteCount; ++n)
                        nones.append(QStringLiteral("none"));
                    row.noteGlide = nones.join(QLatin1Char(' '));
                }
            }
        }

        auto result = CsvAdapter::writeRows(csvPath, rows);
        if (!result.ok()) {
            return Result<void>::Error(result.error());
        }
        return Result<void>::Ok();
    }

} // namespace dstools