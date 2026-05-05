#include "ExportService.h"
#include "PitchExtractionService.h"

#include <dstools/IEditorDataSource.h>
#include <dstools/DsDocument.h>
#include <dstools/PhNumCalculator.h>
#include <dstools/ProjectPaths.h>

#include <hubert-infer/Hfa.h>
#include <rmvpe-infer/Rmvpe.h>
#include <game-infer/Game.h>

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
            if (layer.name == QStringLiteral("grapheme"))
                hasGrapheme = true;
            else if (layer.name.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive))
                hasPhoneme = true;
            else if (layer.name == QStringLiteral("ph_num"))
                hasPhNum = true;
            else if (layer.name == QStringLiteral("midi"))
                hasMidi = true;
        }
        for (const auto &curve : doc.curves) {
            if (curve.name == QStringLiteral("pitch"))
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

        auto dirty = source->dirtyLayers(id);
        if (!dirty.isEmpty())
            ++result.dirtyCount;
    }

    return result;
}

void ExportService::autoCompleteSlice(IEditorDataSource *source, const QString &sliceId,
                                       HFA::HFA *hfa, Rmvpe::Rmvpe *rmvpe,
                                       Game::Game *game, PhNumCalculator *phNumCalc) {
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
        if (layer.name == QStringLiteral("grapheme")) {
            hasGrapheme = true;
            for (const auto &b : layer.boundaries) {
                if (!b.text.isEmpty())
                    graphemeTexts << b.text;
            }
        } else if (layer.name.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive)) {
            hasPhoneme = true;
            for (const auto &b : layer.boundaries) {
                if (!b.text.isEmpty())
                    phonemeTexts << b.text;
            }
        } else if (layer.name == QStringLiteral("ph_num")) {
            hasPhNum = true;
        } else if (layer.name == QStringLiteral("midi")) {
            hasMidi = true;
        }
    }

    for (const auto &curve : doc.curves) {
        if (curve.name == QStringLiteral("pitch"))
            hasPitch = true;
    }

    QString audioPath = source->validatedAudioPath(sliceId);

    if (!hasPhoneme && hasGrapheme && hfa && hfa->isOpen() && !audioPath.isEmpty()) {
        HFA::WordList words;
        for (const auto &text : graphemeTexts) {
            HFA::Word word;
            word.text = text.toStdString();
            HFA::Phone p;
            p.text = text.toStdString();
            word.phones.push_back(p);
            words.push_back(word);
        }

        std::vector<std::string> nonSpeechPh = {"AP", "SP"};
        auto faResult = hfa->recognize(audioPath.toStdWString(), "zh", nonSpeechPh, words);
        if (faResult) {
            IntervalLayer phonemeLayer;
            phonemeLayer.name = QStringLiteral("phoneme");
            phonemeLayer.type = QStringLiteral("interval");
            int id = 1;
            for (const auto &word : words) {
                for (const auto &phone : word.phones) {
                    Boundary b;
                    b.id = id++;
                    b.pos = phone.start;
                    b.text = QString::fromStdString(phone.text);
                    phonemeLayer.boundaries.push_back(std::move(b));
                }
            }
            if (!phonemeLayer.boundaries.empty()) {
                Boundary endB;
                endB.id = id;
                endB.pos = words.back().phones.back().end;
                endB.text = QString();
                phonemeLayer.boundaries.push_back(std::move(endB));
            }

            bool replaced = false;
            for (auto &layer : doc.layers) {
                if (layer.name == QStringLiteral("phoneme")) {
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
                if (layer.name.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive)) {
                    for (const auto &b : layer.boundaries) {
                        if (!b.text.isEmpty())
                            phonemeTexts << b.text;
                    }
                }
            }
        }
    }

    if (hasPhoneme && !hasPhNum && !phonemeTexts.isEmpty() && phNumCalc) {
        QString phNumStr;
        QString error;
        if (phNumCalc->calculate(phonemeTexts.join(' '), phNumStr, error)) {
            IntervalLayer phNumLayer;
            phNumLayer.name = QStringLiteral("ph_num");
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
                if (layer.name == QStringLiteral("ph_num")) {
                    layer = phNumLayer;
                    replaced = true;
                    break;
                }
            }
            if (!replaced)
                doc.layers.push_back(std::move(phNumLayer));
        }
    }

    if (!hasPitch && rmvpe && rmvpe->is_open() && !audioPath.isEmpty()) {
        auto pitchResult = PitchExtractionService::extractPitch(rmvpe, audioPath);
        if (!pitchResult.f0Mhz.empty()) {
            PitchExtractionService::applyPitchToDocument(doc, pitchResult.f0Mhz, pitchResult.timestep);
        }
    }

    if (!hasMidi && game && game->isOpen() && !audioPath.isEmpty()) {
        auto midiResult = PitchExtractionService::transcribeMidi(game, audioPath);
        if (!midiResult.notes.empty()) {
            PitchExtractionService::applyMidiToDocument(doc, midiResult.notes);
        }
    }

    (void) source->saveSlice(sliceId, doc);
}

ExportResult ExportService::exportDataset(IEditorDataSource *source, const ExportOptions &options,
                                           HFA::HFA *, Rmvpe::Rmvpe *,
                                           Game::Game *, PhNumCalculator *) {
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
                QString dstPath = wavsDir + QStringLiteral("/") + dstName;
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
                if (layer.name.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive)) {
                    sentence["ph_seq"] = texts;
                    std::vector<double> durs;
                    for (size_t i = 1; i < positions.size(); ++i)
                        durs.push_back(positions[i] - positions[i - 1]);
                    sentence["ph_dur"] = durs;
                } else if (layer.name.contains(QStringLiteral("note"), Qt::CaseInsensitive)) {
                    sentence["note_seq"] = texts;
                }
            }
            dsDoc.sentences().push_back(sentence);
            auto saveResult = dsDoc.saveFile(dsDir + QStringLiteral("/") + sliceId + QStringLiteral(".ds"));
            if (!saveResult)
                ++result.errorCount;
        }

        if (options.exportCsv) {
            TranscriptionRow row;
            row.name = sliceId;
            for (const auto &layer : doc.layers) {
                if (layer.name.contains(QStringLiteral("phoneme"), Qt::CaseInsensitive)) {
                    QStringList phs, durs;
                    for (size_t i = 0; i < layer.boundaries.size(); ++i) {
                        phs << layer.boundaries[i].text;
                        if (i + 1 < layer.boundaries.size()) {
                            double dur = layer.boundaries[i + 1].pos - layer.boundaries[i].pos;
                            durs << QString::number(dur, 'f', 5);
                        }
                    }
                    row.phSeq = phs.join(' ');
                    row.phDur = durs.join(' ');
                }
                if (layer.name.contains(QStringLiteral("note"), Qt::CaseInsensitive)) {
                    QStringList notes;
                    for (const auto &b : layer.boundaries)
                        notes << b.text;
                    row.noteSeq = notes.join(' ');
                }
            }
            if (!row.phSeq.isEmpty())
                csvRows.push_back(std::move(row));
        }

        ++result.exportedCount;
    }

    if (options.exportCsv && !csvRows.empty()) {
        QString csvError;
        if (!TranscriptionCsv::write(options.outputDir + QStringLiteral("/transcriptions.csv"),
                                     csvRows, csvError))
            ++result.errorCount;
    }

    return result;
}

} // namespace dstools
