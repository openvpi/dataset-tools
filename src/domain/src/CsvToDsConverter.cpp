#include <dstools/CsvToDsConverter.h>
#include <dstools/DsDocument.h>
#include <dstools/TranscriptionCsv.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <sndfile.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>

namespace dstools {

namespace {

    /// Format a single float to 6 decimal places, strip trailing zeros.
    /// Matches Python: str(round(d, 6))
    QString formatDur6(double d) {
        QString s = QString::number(d, 'f', 6);
        if (s.contains(QLatin1Char('.'))) {
            while (s.endsWith(QLatin1Char('0')))
                s.chop(1);
            if (s.endsWith(QLatin1Char('.')))
                s.chop(1);
        }
        return s;
    }

    /// Re-round a space-separated duration string to 6 decimal places.
    /// Matches Python: " ".join(str(round(d, 6)) for d in durations)
    QString reroundDurs(const QString &durStr) {
        if (durStr.isEmpty())
            return durStr;
        const QStringList parts = durStr.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        QStringList result;
        result.reserve(parts.size());
        for (const QString &p : parts) {
            result.append(formatDur6(p.toDouble()));
        }
        return result.join(QLatin1Char(' '));
    }

    /// Format F0 sequence to 1 decimal place with C locale.
    /// Matches Python: " ".join(map("{:.1f}".format, f0))
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

    /// Built-in F0 extraction using autocorrelation pitch detection.
    /// Used as fallback when no external F0 callback is provided.
    bool extractF0Builtin(const QString &wavPath, int sampleRate, int hopSize,
                          std::vector<float> &f0, QString &error) {
        // Open WAV file with libsndfile
        SF_INFO sfInfo{};
        SNDFILE *file = sf_open(wavPath.toUtf8().constData(), SFM_READ, &sfInfo);
        if (!file) {
            error = QStringLiteral("Failed to open WAV file: ") + wavPath;
            return false;
        }

        // Read all samples (mix to mono if needed)
        const sf_count_t totalFrames = sfInfo.frames;
        const int channels = sfInfo.channels;
        std::vector<float> raw(static_cast<size_t>(totalFrames * channels));
        sf_readf_float(file, raw.data(), totalFrames);
        sf_close(file);

        // Mix to mono
        std::vector<float> mono(static_cast<size_t>(totalFrames));
        if (channels == 1) {
            mono = std::move(raw);
        } else {
            for (sf_count_t i = 0; i < totalFrames; ++i) {
                float sum = 0.0f;
                for (int ch = 0; ch < channels; ++ch)
                    sum += raw[static_cast<size_t>(i * channels + ch)];
                mono[static_cast<size_t>(i)] = sum / static_cast<float>(channels);
            }
        }

        // Autocorrelation-based pitch detection per hop frame
        const float minF0 = 50.0f;   // Hz
        const float maxF0 = 1100.0f;  // Hz
        const int minLag = sampleRate / static_cast<int>(maxF0);
        const int maxLag = sampleRate / static_cast<int>(minF0);
        const int windowSize = maxLag * 2;
        const int numFrames = static_cast<int>((totalFrames - windowSize) / hopSize) + 1;

        f0.resize(std::max(numFrames, 0), 0.0f);

        for (int frame = 0; frame < numFrames; ++frame) {
            const int offset = frame * hopSize;
            if (offset + windowSize > static_cast<int>(totalFrames))
                break;

            const float *win = mono.data() + offset;

            // Compute energy of the window
            float energy = 0.0f;
            for (int i = 0; i < windowSize; ++i)
                energy += win[i] * win[i];

            // Silence threshold — unvoiced
            if (energy < 1e-6f * windowSize) {
                f0[static_cast<size_t>(frame)] = 0.0f;
                continue;
            }

            // Normalized autocorrelation (YIN-like difference function)
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

            // Voicing threshold
            if (bestCorr > 0.5f && bestLag > 0) {
                f0[static_cast<size_t>(frame)] =
                    static_cast<float>(sampleRate) / static_cast<float>(bestLag);
            } else {
                f0[static_cast<size_t>(frame)] = 0.0f;
            }
        }

        return true;
    }

} // anonymous namespace


bool CsvToDsConverter::convert(const Options &opts,
                                F0Callback f0Callback,
                                ProgressCallback progress,
                                QString &error) {
    std::vector<TranscriptionRow> rows;
    if (!TranscriptionCsv::read(opts.csvPath, rows, error))
        return false;

    return convertFromMemory(rows, opts, f0Callback, progress, error);
}


bool CsvToDsConverter::convertFromMemory(const std::vector<TranscriptionRow> &rows,
                                          const Options &opts,
                                          F0Callback f0Callback,
                                          ProgressCallback progress,
                                          QString &error) {
    QDir outDir(opts.outputDir);
    if (!outDir.exists()) {
        outDir.mkpath(QStringLiteral("."));
    }

    const int total = static_cast<int>(rows.size());

    for (int i = 0; i < total; ++i) {
        const auto &row = rows[i];

        if (progress)
            progress(i + 1, total, row.name);

        const QString wavPath =
            opts.wavsDir + QDir::separator() + row.name + QStringLiteral(".wav");
        const QString dsPath =
            opts.outputDir + QDir::separator() + row.name + QStringLiteral(".ds");

        // F0 extraction
        std::vector<float> f0;
        if (f0Callback) {
            if (!f0Callback(wavPath, f0, error))
                return false;
        } else {
            // Built-in fallback: autocorrelation pitch detection via libsndfile
            if (!extractF0Builtin(wavPath, opts.sampleRate, opts.hopSize, f0, error))
                return false;
        }

        // Build sentence JSON
        nlohmann::json sentence;
        sentence["offset"] = 0.0;

        // text = ph_seq (matching Python)
        sentence["text"] = row.phSeq.toStdString();
        sentence["ph_seq"] = row.phSeq.toStdString();

        // ph_dur rounded to 6 decimals
        sentence["ph_dur"] = reroundDurs(row.phDur).toStdString();

        // ph_num (optional)
        if (!row.phNum.isEmpty())
            sentence["ph_num"] = row.phNum.toStdString();

        // note_seq (optional)
        if (!row.noteSeq.isEmpty())
            sentence["note_seq"] = row.noteSeq.toStdString();

        // note_dur rounded to 6 decimals (optional)
        if (!row.noteDur.isEmpty())
            sentence["note_dur"] = reroundDurs(row.noteDur).toStdString();

        // note_slur (optional)
        if (!row.noteSlur.isEmpty())
            sentence["note_slur"] = row.noteSlur.toStdString();

        // note_glide (optional)
        if (!row.noteGlide.isEmpty())
            sentence["note_glide"] = row.noteGlide.toStdString();

        // f0_seq — empty string if no callback provided
        sentence["f0_seq"] = formatF0Seq(f0).toStdString();

        // f0_timestep as STRING (matching Python behavior)
        sentence["f0_timestep"] = std::to_string(
            static_cast<double>(opts.hopSize) / opts.sampleRate);

        // Write via DsDocument
        DsDocument doc;
        doc.sentences().push_back(std::move(sentence));
        if (!doc.save(dsPath, error))
            return false;
    }

    return true;
}


bool CsvToDsConverter::dsToCsv(const QString &dsDir,
                                const QString &csvPath,
                                QString &error) {
    QDir dir(dsDir);
    const QFileInfoList entries = dir.entryInfoList(
        {QStringLiteral("*.ds")}, QDir::Files, QDir::Name);

    std::vector<TranscriptionRow> rows;
    bool anyWithGlide = false;

    // Pass 1: .ds files WITH corresponding .wav — take sentence[0]
    for (const QFileInfo &entry : entries) {
        const QString wavPath = entry.absolutePath() + QDir::separator() +
                                entry.completeBaseName() + QStringLiteral(".wav");
        if (!QFile::exists(wavPath))
            continue;

        DsDocument doc = DsDocument::load(entry.absoluteFilePath(), error);
        if (doc.isEmpty())
            return false;

        const auto &item = doc.sentence(0);

        TranscriptionRow row;
        row.name = entry.completeBaseName();
        row.phSeq = QString::fromStdString(item.value("ph_seq", ""));
        row.phDur = reroundDurs(QString::fromStdString(item.value("ph_dur", "")));
        row.phNum = QString::fromStdString(item.value("ph_num", ""));
        row.noteSeq = QString::fromStdString(item.value("note_seq", ""));
        row.noteDur = reroundDurs(QString::fromStdString(item.value("note_dur", "")));
        // NOTE: Python does NOT output note_slur — row.noteSlur stays empty

        if (item.contains("note_glide")) {
            anyWithGlide = true;
            row.noteGlide = QString::fromStdString(item.value("note_glide", ""));
        }

        rows.push_back(std::move(row));
    }

    // Pass 2: .ds files WITHOUT corresponding .wav — multi-sentence support
    for (const QFileInfo &entry : entries) {
        const QString wavPath = entry.absolutePath() + QDir::separator() +
                                entry.completeBaseName() + QStringLiteral(".wav");
        if (QFile::exists(wavPath))
            continue; // already processed in Pass 1

        DsDocument doc = DsDocument::load(entry.absoluteFilePath(), error);
        if (doc.isEmpty())
            return false;

        const int count = doc.sentenceCount();
        for (int idx = 0; idx < count; ++idx) {
            const auto &subDs = doc.sentence(idx);

            TranscriptionRow row;
            // Python naming: "{stem}#{idx}" if multi-sentence
            if (count > 1) {
                row.name = entry.completeBaseName() +
                           QStringLiteral("#") + QString::number(idx);
            } else {
                row.name = entry.completeBaseName();
            }

            row.phSeq = QString::fromStdString(subDs.value("ph_seq", ""));
            row.phDur = reroundDurs(QString::fromStdString(subDs.value("ph_dur", "")));
            row.phNum = QString::fromStdString(subDs.value("ph_num", ""));
            row.noteSeq = QString::fromStdString(subDs.value("note_seq", ""));
            row.noteDur = reroundDurs(QString::fromStdString(subDs.value("note_dur", "")));

            if (subDs.contains("note_glide")) {
                anyWithGlide = true;
                row.noteGlide = QString::fromStdString(subDs.value("note_glide", ""));
            }

            rows.push_back(std::move(row));
        }
    }

    // If any item has note_glide, fill missing ones with "none" repeated
    if (anyWithGlide) {
        for (auto &row : rows) {
            if (row.noteGlide.isEmpty() && !row.noteSeq.isEmpty()) {
                const int noteCount =
                    row.noteSeq.split(QLatin1Char(' '), Qt::SkipEmptyParts).size();
                QStringList nones;
                nones.reserve(noteCount);
                for (int n = 0; n < noteCount; ++n)
                    nones.append(QStringLiteral("none"));
                row.noteGlide = nones.join(QLatin1Char(' '));
            }
        }
    }

    // Write CSV
    return TranscriptionCsv::write(csvPath, rows, error);
}

} // namespace dstools
