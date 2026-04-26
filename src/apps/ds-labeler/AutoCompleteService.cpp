#include "AutoCompleteService.h"

#include <dstools/PhNumCalculator.h>
#include <dstools/PitchUtils.h>

#include "InferBridge.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

namespace dstools {

AutoCompleteResult autoCompleteSlice(DsTextDocument doc,
                                     const QString &audioPath,
                                     HFA::HFA *hfa,
                                     Rmvpe::Rmvpe *rmvpe,
                                     Game::Game *game,
                                     PhNumCalculator *phNumCalc) {
    AutoCompleteResult result;
    result.doc = std::move(doc);

    bool hasPhoneme = false;
    bool hasPhNum = false;
    bool hasPitch = false;
    bool hasMidi = false;
    bool hasGrapheme = false;
    QStringList graphemeTexts;
    QStringList phonemeTexts;

    for (const auto &layer : result.doc.layers) {
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

    for (const auto &curve : result.doc.curves) {
        if (curve.name == QStringLiteral("pitch"))
            hasPitch = true;
    }

    if (!hasPhoneme && hasGrapheme && hfa && hfa->isOpen() && !audioPath.isEmpty()) {
        std::string lyricsText;
        for (const auto &text : graphemeTexts) {
            if (!lyricsText.empty())
                lyricsText += " ";
            lyricsText += text.toStdString();
        }

        HFA::WordList words;
        std::vector<std::string> nonSpeechPh = {"AP", "SP"};
        auto faResult = hfa->recognize(audioPath.toStdWString(), "zh", nonSpeechPh, lyricsText, words);
        if (faResult) {
            IntervalLayer phonemeLayer;
            phonemeLayer.name = QStringLiteral("phoneme");
            phonemeLayer.type = QStringLiteral("interval");
            int id = 1;
            for (const auto &word : words) {
                if (word.text == "SP" || word.text == "AP")
                    continue;
                for (const auto &phone : word.phones) {
                    if (phone.text == "SP" || phone.text == "AP")
                        continue;
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
            for (auto &layer : result.doc.layers) {
                if (layer.name == QStringLiteral("phoneme")) {
                    layer = std::move(phonemeLayer);
                    replaced = true;
                    break;
                }
            }
            if (!replaced)
                result.doc.layers.push_back(std::move(phonemeLayer));
            result.modified = true;
            hasPhoneme = true;

            phonemeTexts.clear();
            for (const auto &layer : result.doc.layers) {
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
        auto calcResult = phNumCalc->calculate(phonemeTexts.join(' '));
        if (calcResult.ok()) {
            const QString &phNumStr = calcResult.value();
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
            for (auto &layer : result.doc.layers) {
                if (layer.name == QStringLiteral("ph_num")) {
                    layer = std::move(phNumLayer);
                    replaced = true;
                    break;
                }
            }
            if (!replaced)
                result.doc.layers.push_back(std::move(phNumLayer));
            result.modified = true;
        }
    }

    if (!hasPitch && rmvpe && rmvpe->is_open() && !audioPath.isEmpty()) {
        std::vector<Rmvpe::RmvpeRes> results;
        auto rmvpeResult = rmvpe->get_f0(audioPath.toStdWString(), 0.01f, results, nullptr);
        if (rmvpeResult && !results.empty()) {
            const auto &res = results[0];
            std::vector<float> f0Mhz(res.f0.size());
            for (size_t i = 0; i < res.f0.size(); ++i)
                f0Mhz[i] = res.f0[i] * 1000.0f;

            float timestep = 0.01f;
            CurveLayer *pitchCurve = nullptr;
            for (auto &curve : result.doc.curves) {
                if (curve.name == QStringLiteral("pitch")) {
                    pitchCurve = &curve;
                    break;
                }
            }
            if (!pitchCurve) {
                result.doc.curves.push_back({});
                pitchCurve = &result.doc.curves.back();
                pitchCurve->name = QStringLiteral("pitch");
            }
            pitchCurve->values = std::move(f0Mhz);
            pitchCurve->timestep = timestep;
            result.modified = true;
        }
    }

    if (!hasMidi && game && game->isOpen() && !audioPath.isEmpty()) {
        std::vector<Game::GameNote> notes;
        auto gameResult = game->getNotes(audioPath.toStdWString(), notes, nullptr);
        if (gameResult && !notes.empty()) {
            IntervalLayer *midiLayer = nullptr;
            for (auto &layer : result.doc.layers) {
                if (layer.name == QStringLiteral("midi")) {
                    midiLayer = &layer;
                    break;
                }
            }
            if (!midiLayer) {
                result.doc.layers.push_back({});
                midiLayer = &result.doc.layers.back();
                midiLayer->name = QStringLiteral("midi");
                midiLayer->type = QStringLiteral("note");
            }

            midiLayer->boundaries.clear();
            int id = 1;
            for (const auto &gn : notes) {
                QJsonObject obj;
                obj["n"] = gn.voiced
                    ? midiToNoteString(static_cast<double>(gn.pitch))
                    : QStringLiteral("rest");
                obj["d"] = static_cast<qint64>(gn.duration * 1000000.0);

                Boundary b;
                b.id = id++;
                b.pos = static_cast<int64_t>(gn.onset * 1000000.0);
                b.text = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
                midiLayer->boundaries.push_back(std::move(b));
            }
            result.modified = true;
        }
    }

    return result;
}

} // namespace dstools