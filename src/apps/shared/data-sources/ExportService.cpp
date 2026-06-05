#include "ExportService.h"
#include "PitchExtractionService.h"

#include <dstools/IEditorDataSource.h>
#include <dstools/DsDocument.h>
#include <dstools/PhNumCalculator.h>
#include <dstools/ProjectPaths.h>
#include <dstools/CsvAdapter.h>
#include <dstools/DsKeys.h>
#include <dsfw/PathUtils.h>

#include <QJsonDocument>
#include <QJsonObject>

#include <dsfw/Log.h>

#include <QDir>
#include <QFile>

namespace dstools {

ExportValidationResult ExportService::validate(IEditorDataSource *source) {
    ExportValidationResult result;
    if (!source)
        return result;

    const auto ids = source->sliceIds();
    result.totalSlices = ids.size();

    for (const auto &id : ids) {
        auto loadResult = source->loadSlice(id);
        if (!loadResult)
            continue;

        const auto &doc = loadResult.value();

        bool hasGrapheme = false;
        bool hasPhoneme = false;
        bool hasPhNum = false;
        bool hasPitch = false;
        bool hasMidi = false;

        for (const auto &layer : doc.layers) {
            if (layer.name == QString::fromUtf8(dstools::keys::layers::grapheme))
                hasGrapheme = true;
            else if (layer.name.contains(QString::fromUtf8(dstools::keys::layers::phoneme), Qt::CaseInsensitive))
                hasPhoneme = true;
            else if (layer.name == QString::fromUtf8(dstools::keys::layers::phNum))
                hasPhNum = true;
            else if (layer.name == QString::fromUtf8(dstools::keys::layers::midi))
                hasMidi = true;
        }
        for (const auto &curve : doc.curves) {
            if (curve.name == QString::fromUtf8(dstools::keys::layers::pitch))
                hasPitch = true;
        }

        if (hasGrapheme)
            ++result.readyForCsv;

        if (doc.meta.editedSteps.contains(QStringLiteral("pitch_review")))
            ++result.readyForDs;

        if (hasGrapheme && !hasPhoneme)
            ++result.missingFa;
        if (hasPhoneme && !hasPhNum)
            ++result.missingPhNum;
        if (!hasPitch)
            ++result.missingPitch;
        if (!hasMidi)
            ++result.missingMidi;

        bool sliceReady = hasGrapheme && hasPhoneme && hasPhNum && hasPitch && hasMidi;
        bool sliceHasAny = hasGrapheme || hasPhoneme || hasPhNum || hasPitch || hasMidi;
        if (sliceReady)
            ++result.readySlices;
        if (!sliceHasAny)
            ++result.missingLayerSlices;

        auto dirty = source->dirtyLayers(id);
        if (!dirty.isEmpty()) {
            ++result.dirtyCount;
            ++result.dirtySlices;
        }

        // ── Column coverage ──
        ++result.colCoverage[0]; // name always present

        if (hasPhoneme && !doc.layers.empty()) {
            const IntervalLayer *phonemeLayer = nullptr;
            const IntervalLayer *phNumLayer = nullptr;
            const IntervalLayer *midiLayer = nullptr;
            for (const auto &layer : doc.layers) {
                if (layer.name.contains(QString::fromUtf8(dstools::keys::layers::phoneme), Qt::CaseInsensitive))
                    phonemeLayer = &layer;
                else if (layer.name == QString::fromUtf8(dstools::keys::layers::phNum))
                    phNumLayer = &layer;
                else if (layer.name == QString::fromUtf8(dstools::keys::layers::midi))
                    midiLayer = &layer;
            }

            if (phonemeLayer && !phonemeLayer->boundaries.empty()) {
                ++result.colCoverage[1];
                if (phonemeLayer->boundaries.size() >= 2)
                    ++result.colCoverage[2];
            }
            if (phNumLayer && !phNumLayer->boundaries.empty())
                ++result.colCoverage[3];
            if (midiLayer && !midiLayer->boundaries.empty()) {
                ++result.colCoverage[4];
                ++result.colCoverage[5];
            }

        }
    }

    return result;
}

void ExportService::autoCompleteSlice(IEditorDataSource *source, const QString &sliceId,
                                       dsfw::infer::IInferenceService *inferService,
                                       PhNumCalculator *phNumCalc) {
    if (!source)
        return;

    auto result = source->loadSlice(sliceId);
    if (!result)
        return;

    DsTextDocument doc = std::move(result.value());

    bool hasPhoneme = false;
    bool hasPhNum = false;
    bool hasPitch = false;
    bool hasMidi = false;
    bool hasGrapheme = false;
    QStringList graphemeTexts;
    QStringList phonemeTexts;

    for (const auto &layer : doc.layers) {
        if (layer.name == QString::fromUtf8(dstools::keys::layers::grapheme)) {
            hasGrapheme = true;
            for (const auto &b : layer.boundaries) {
                if (!b.text.isEmpty())
                    graphemeTexts << b.text;
            }
        } else if (layer.name.contains(QString::fromUtf8(dstools::keys::layers::phoneme), Qt::CaseInsensitive)) {
            hasPhoneme = true;
            for (const auto &b : layer.boundaries) {
                if (!b.text.isEmpty())
                    phonemeTexts << b.text;
            }
        } else if (layer.name == QString::fromUtf8(dstools::keys::layers::phNum)) {
            hasPhNum = true;
        } else if (layer.name == QString::fromUtf8(dstools::keys::layers::midi)) {
            hasMidi = true;
        }
    }

    for (const auto &curve : doc.curves) {
        if (curve.name == QString::fromUtf8(dstools::keys::layers::pitch))
            hasPitch = true;
    }

    QString audioPath = source->validatedAudioPath(sliceId);

    if (!hasPhoneme && hasGrapheme && inferService && inferService->hasForcedAlignment() && !audioPath.isEmpty()) {
        std::string lyricsText;
        for (const auto &text : graphemeTexts) {
            if (!lyricsText.empty()) lyricsText += " ";
            lyricsText += text.toStdString();
        }

        std::vector<dsfw::infer::AlignedWord> words;
        std::vector<std::string> nonSpeechPh = {"AP", "SP"};
        auto audioFilePath = std::filesystem::path(audioPath.toStdWString());
        auto faResult = inferService->runForcedAlignment(audioFilePath, "zh", nonSpeechPh, lyricsText, words);
        if (faResult) {
            IntervalLayer phonemeLayer;
            phonemeLayer.name = QString::fromUtf8(dstools::keys::layers::phoneme);
            phonemeLayer.type = QStringLiteral("interval");
            int id = 1;
            for (const auto &word : words) {
                for (const auto &phone : word.phones) {
                    Boundary b;
                    b.id = id++;
                    b.pos = secToUs(phone.start);
                    b.text = QString::fromStdString(phone.text);
                    phonemeLayer.boundaries.push_back(std::move(b));
                }
            }
            if (!phonemeLayer.boundaries.empty()) {
                Boundary endB;
                endB.id = id;
                endB.pos = secToUs(words.back().phones.back().end);
                endB.text = QString();
                phonemeLayer.boundaries.push_back(std::move(endB));
            }

            bool replaced = false;
            for (auto &layer : doc.layers) {
                if (layer.name == QString::fromUtf8(dstools::keys::layers::phoneme)) {
                    layer = phonemeLayer;
                    replaced = true;
                    break;
                }
            }
            if (!replaced)
                doc.layers.push_back(std::move(phonemeLayer));

            hasPhoneme = true;
            phonemeTexts.clear();
            for (const auto &layer : doc.layers) {
                if (layer.name.contains(QString::fromUtf8(dstools::keys::layers::phoneme), Qt::CaseInsensitive)) {
                    for (const auto &b : layer.boundaries) {
                        if (!b.text.isEmpty())
                            phonemeTexts << b.text;
                    }
                }
            }
        }
    }

    if (hasPhoneme && !hasPhNum && !phonemeTexts.isEmpty() && phNumCalc) {
        auto calcResult = phNumCalc->calculate(phonemeTexts.join(' '));
        if (calcResult.ok()) {
            const QString &phNumStr = calcResult.value();
            IntervalLayer phNumLayer;
            phNumLayer.name = QString::fromUtf8(dstools::keys::layers::phNum);
            phNumLayer.type = QStringLiteral("attribute");
            const auto parts = phNumStr.split(QChar(' '), Qt::SkipEmptyParts);
            int id = 1;
            for (const auto &part : parts) {
                Boundary b;
                b.id = id++;
                b.text = part;
                phNumLayer.boundaries.push_back(std::move(b));
            }

            bool replaced = false;
            for (auto &layer : doc.layers) {
                if (layer.name == QString::fromUtf8(dstools::keys::layers::phNum)) {
                    layer = phNumLayer;
                    replaced = true;
                    break;
                }
            }
            if (!replaced)
                doc.layers.push_back(std::move(phNumLayer));
        }
    }

    if (!hasPitch && inferService && inferService->hasPitchExtraction() && !audioPath.isEmpty()) {
        dsfw::infer::PitchResult pitchResult;
        auto audioFilePath = std::filesystem::path(audioPath.toStdWString());
        bool pitchOk = inferService->runPitchExtraction(audioFilePath, 0.01f, pitchResult);
        if (pitchOk && !pitchResult.f0.empty()) {
            std::vector<float> f0Mhz(pitchResult.f0.size());
            for (size_t i = 0; i < pitchResult.f0.size(); ++i)
                f0Mhz[i] = pitchResult.f0[i] * 1000.0f;
            PitchExtractionService::applyPitchToDocument(doc, f0Mhz, pitchResult.timestep);
        }
    }

    if (!hasMidi && inferService && inferService->hasMidiTranscription() && !audioPath.isEmpty()) {
        std::vector<dsfw::infer::MidiNote> notes;
        auto audioFilePath = std::filesystem::path(audioPath.toStdWString());
        bool midiOk = inferService->runMidiTranscription(audioFilePath, notes);
        if (midiOk && !notes.empty()) {
            PitchExtractionService::applyMidiToDocument(doc, notes);
        }
    }

    auto saveResult = source->saveSlice(sliceId, doc);
    if (!saveResult) {
        DSFW_LOG_ERROR("export", ("Failed to save slice after autoComplete: " + sliceId.toStdString() + " - " + saveResult.error()).c_str());
    }
}

ExportResult ExportService::exportDataset(IEditorDataSource *source, const ExportOptions &options,
                                           dsfw::infer::IInferenceService *,
                                           PhNumCalculator *) {
    ExportResult result;
    if (!source)
        return result;

    const auto sliceIds = source->sliceIds();
    if (sliceIds.isEmpty())
        return result;

    QDir().mkpath(options.outputDir);
    const QString wavsDir = ProjectPaths::wavsDir(options.outputDir);
    const QString dsDir = ProjectPaths::dsDir(options.outputDir);
    if (options.exportWavs)
        QDir().mkpath(wavsDir);
    if (options.exportDs)
        QDir().mkpath(dsDir);

    std::vector<TranscriptionRow> csvRows;

    for (const auto &sliceId : sliceIds) {
        auto loadResult = source->loadSlice(sliceId);
        if (!loadResult) {
            ++result.errorCount;
            continue;
        }

        const DsTextDocument &doc = loadResult.value();

        if (options.exportWavs) {
            QString srcAudio = source->validatedAudioPath(sliceId);
            if (!srcAudio.isEmpty()) {
                QString dstName = sliceId + QStringLiteral(".wav");
                QString dstPath = QDir(wavsDir).filePath(dstName);
                if (!QFile::copy(srcAudio, dstPath)) {
                    if (!QFile::remove(dstPath) || !QFile::copy(srcAudio, dstPath))
                        ++result.errorCount;
                }
            }
        }

        if (options.exportDs && doc.meta.editedSteps.contains(QStringLiteral("pitch_review"))) {
            DsDocument dsDoc;
            nlohmann::json sentence;
            sentence["name"] = sliceId.toStdString();
            for (const auto &layer : doc.layers) {
                std::vector<double> positions;
                std::vector<std::string> texts;
                for (const auto &b : layer.boundaries) {
                    positions.push_back(b.pos);
                    texts.push_back(b.text.toStdString());
                }
                if (layer.name.contains(QString::fromUtf8(dstools::keys::layers::phoneme), Qt::CaseInsensitive)) {
                    sentence["ph_seq"] = texts;
                    std::vector<double> durs;
                    for (size_t i = 1; i < positions.size(); ++i)
                        durs.push_back(positions[i] - positions[i - 1]);
                    sentence[std::string(dstools::keys::csv::phDur)] = durs;
                } else if (layer.name.contains(QString::fromUtf8(dstools::keys::layers::note), Qt::CaseInsensitive)) {
                    std::vector<std::string> noteNames;
                    for (const auto &b : layer.boundaries) {
                        QJsonParseError err;
                        auto jdoc = QJsonDocument::fromJson(b.text.toUtf8(), &err);
                        if (err.error == QJsonParseError::NoError && jdoc.isObject())
                            noteNames.push_back(jdoc.object()["n"].toString().toStdString());
                        else if (!b.text.isEmpty())
                            noteNames.push_back(b.text.toStdString());
                    }
                    sentence[std::string(dstools::keys::csv::noteSeq)] = noteNames;
                }
            }
            dsDoc.addRawSentence(sentence.dump());
            auto saveResult = dsDoc.saveFile(QDir(dsDir).filePath(sliceId + QStringLiteral(".ds")));
            if (!saveResult)
                ++result.errorCount;
        }

        if (options.exportCsv) {
            TranscriptionRow row;
            row.name = sliceId;

            const IntervalLayer *phonemeLayer = nullptr;
            const IntervalLayer *graphemeLayer = nullptr;
            const IntervalLayer *midiLayer = nullptr;
            const IntervalLayer *phNumLayer = nullptr;
            for (const auto &layer : doc.layers) {
                if (layer.name.contains(QString::fromUtf8(dstools::keys::layers::phoneme), Qt::CaseInsensitive))
                    phonemeLayer = &layer;
                else if (layer.name == QString::fromUtf8(dstools::keys::layers::grapheme))
                    graphemeLayer = &layer;
                else if (layer.name.contains(QString::fromUtf8(dstools::keys::layers::midi), Qt::CaseInsensitive)
                         || layer.name.contains(QString::fromUtf8(dstools::keys::layers::note), Qt::CaseInsensitive))
                    midiLayer = &layer;
                else if (layer.name == QString::fromUtf8(dstools::keys::layers::phNum))
                    phNumLayer = &layer;
            }

            if (phonemeLayer) {
                QStringList phs, durs;
                for (size_t i = 0; i < phonemeLayer->boundaries.size(); ++i) {
                    phs << phonemeLayer->boundaries[i].text;
                    if (i + 1 < phonemeLayer->boundaries.size()) {
                        double dur = usToSec(phonemeLayer->boundaries[i + 1].pos - phonemeLayer->boundaries[i].pos);
                        durs << QString::number(dur, 'f', 6);
                    }
                }
                row.phSeq = phs.join(' ');
                row.phDur = durs.join(' ');

                if (graphemeLayer && !graphemeLayer->boundaries.empty()) {
                    QStringList spans;
                    int wordIdx = 1;
                    for (size_t i = 0; i < phonemeLayer->boundaries.size(); ++i) {
                        const auto &ph = phonemeLayer->boundaries[i];
                        if (ph.text.isEmpty())
                            continue;
                        while (wordIdx < static_cast<int>(graphemeLayer->boundaries.size()) &&
                               ph.pos >= graphemeLayer->boundaries[wordIdx].pos) {
                            ++wordIdx;
                        }
                        spans << QString::number(wordIdx);
                    }
                    row.wordSpan = spans.join(' ');
                }
            }

            if (phNumLayer) {
                QStringList nums;
                for (const auto &b : phNumLayer->boundaries) {
                    if (!b.text.isEmpty())
                        nums << b.text;
                }
                row.phNum = nums.join(' ');
            }

            if (midiLayer) {
                QStringList notes;
                QStringList noteDurs;
                for (const auto &b : midiLayer->boundaries) {
                    QJsonParseError err;
                    auto jdoc = QJsonDocument::fromJson(b.text.toUtf8(), &err);
                    if (err.error == QJsonParseError::NoError && jdoc.isObject()) {
                        auto obj = jdoc.object();
                        notes << obj["n"].toString();
                        double durSec = usToSec(static_cast<TimePos>(obj["d"].toDouble()));
                        noteDurs << QString::number(durSec, 'f', 6);
                    } else {
                        notes << b.text;
                        noteDurs << QStringLiteral("0");
                    }
                }
                row.noteSeq = notes.join(' ');
                row.noteDur = noteDurs.join(' ');
            }

            if (!row.phSeq.isEmpty())
                csvRows.push_back(std::move(row));
        }

        ++result.exportedCount;
    }

    if (options.exportCsv && !csvRows.empty()) {
        auto writeResult = CsvAdapter::writeRows(
            options.outputDir + QStringLiteral("/transcriptions.csv"), csvRows);
        if (!writeResult.ok())
            ++result.errorCount;
    }

    return result;
}

} // namespace dstools
