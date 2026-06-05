#include "DsFileAdapter.h"
#include "PhonemeLayerBuilder.h"

#include <dsfw/ConfigTypes.h>
#include <dstools/DsDocument.h>
#include <dstools/TimePos.h>

namespace dstools {

namespace {

Result<void> validateLayerData(const std::map<QString, LayerData> &layers) {
    if (layers.empty())
        return Result<void>::Error("import produced no layers");
    for (const auto &[key, val] : layers) {
        if (val.empty())
            return Result<void>::Error(QString("layer '%1' is empty").arg(key).toStdString());
        try {
            const auto j = val.toJson();
            if (j.is_null())
                return Result<void>::Error(QString("layer '%1' contains null JSON").arg(key).toStdString());
        } catch (const std::exception &e) {
            return Result<void>::Error(QString("layer '%1' JSON parse error: %2").arg(key, e.what()).toStdString());
        }
    }
    return Result<void>::Ok();
}

} // anonymous namespace

Result<void> DsFileAdapter::importToLayers(const QString &filePath,
                                           std::map<QString, LayerData> &layers,
                                           const ProcessorConfig &config) {
    auto docResult = DsDocument::loadFile(filePath);
    if (!docResult.ok())
        return Result<void>::Error(docResult.error());

    auto &doc = docResult.value();
    if (doc.isEmpty())
        return Result<void>::Error("DS file contains no sentences");

    int idx = 0;
    if (config.contains(QStringLiteral("index")))
        idx = static_cast<int>(configValueInt(config, QStringLiteral("index")));
    if (idx < 0 || idx >= doc.sentenceCount())
        return Result<void>::Error("Sentence index out of range");

    const auto sv = doc.sentenceView(idx);

    const double offset = sv.offset;

    std::map<QString, LayerData> temp;

    if (!sv.phSeq.isEmpty() && !sv.phDur.isEmpty()) {
        const QStringList phones = sv.phSeq.split(' ', Qt::SkipEmptyParts);
        const QStringList durs = sv.phDur.split(' ', Qt::SkipEmptyParts);

        auto boundaries = buildBoundaries(phones, durs, secToUs(offset));
        temp[QStringLiteral("phoneme")] = LayerData::fromJson({{"name", "phoneme"}, {"boundaries", boundaries.toJson()}});
    }

    if (!sv.noteSeq.isEmpty() && !sv.noteDur.isEmpty()) {
        const QStringList notes = sv.noteSeq.split(' ', Qt::SkipEmptyParts);
        const QStringList noteDurs = sv.noteDur.split(' ', Qt::SkipEmptyParts);

        auto boundaries = buildBoundaries(notes, noteDurs, secToUs(offset));
        temp[QStringLiteral("midi")] = LayerData::fromJson({{"name", "midi"}, {"boundaries", boundaries.toJson()}});
    }

    if (!sv.f0Seq.isEmpty() && sv.f0Timestep > 0.0) {
        const double timestep = sv.f0Timestep;
        const QStringList f0Values = sv.f0Seq.split(' ', Qt::SkipEmptyParts);
        std::vector<double> f0Seq;
        f0Seq.reserve(f0Values.size());
        for (const auto &v : f0Values)
            f0Seq.push_back(v.toDouble());
        temp[QStringLiteral("pitch")] = LayerData::fromJson({
            {"name", "pitch"},
            {"f0_seq", f0Seq},
            {"f0_timestep", timestep},
            {"offset", offset}});
    }

    auto validation = validateLayerData(temp);
    if (!validation.ok())
        return validation;

    layers = std::move(temp);
    return Result<void>::Ok();
}

static std::map<QString, nlohmann::json> layersToJson(const std::map<QString, LayerData> &layers) {
    std::map<QString, nlohmann::json> result;
    for (const auto &[key, val] : layers)
        result[key] = val.toJson();
    return result;
}

Result<void> DsFileAdapter::exportFromLayers(const std::map<QString, LayerData> &layers,
                                             const QString &outputPath,
                                             const ProcessorConfig &config) {
    const auto lyr = layersToJson(layers);
    nlohmann::json sentence;

    const double offset = configValueDouble(config, QStringLiteral("offset"));
    sentence["offset"] = offset;

    auto phIt = lyr.find(QStringLiteral("phoneme"));
    if (phIt != lyr.end()) {
        const auto &boundaries = phIt->second["boundaries"];
        QStringList phones;
        QStringList durs;
        for (size_t i = 0; i + 1 < boundaries.size(); ++i) {
            phones.append(QString::fromStdString(boundaries[i].value("text", "")));
            const TimePos posA = boundaries[i].value("pos", int64_t(0));
            const TimePos posB = boundaries[i + 1].value("pos", int64_t(0));
            durs.append(QString::number(usToSec(posB - posA), 'f', 6));
        }
        sentence["ph_seq"] = phones.join(' ').toStdString();
        sentence["ph_dur"] = durs.join(' ').toStdString();
    }

    auto midiIt = lyr.find(QStringLiteral("midi"));
    if (midiIt != lyr.end()) {
        const auto &boundaries = midiIt->second["boundaries"];
        QStringList notes;
        QStringList noteDurs;
        for (size_t i = 0; i + 1 < boundaries.size(); ++i) {
            notes.append(QString::fromStdString(boundaries[i].value("text", "")));
            const TimePos posA = boundaries[i].value("pos", int64_t(0));
            const TimePos posB = boundaries[i + 1].value("pos", int64_t(0));
            noteDurs.append(QString::number(usToSec(posB - posA), 'f', 6));
        }
        sentence["note_seq"] = notes.join(' ').toStdString();
        sentence["note_dur"] = noteDurs.join(' ').toStdString();
    }

    auto pitchIt = lyr.find(QStringLiteral("pitch"));
    if (pitchIt != lyr.end()) {
        if (pitchIt->second.contains("f0_seq"))
            sentence["f0_seq"] = pitchIt->second["f0_seq"];
        if (pitchIt->second.contains("f0_timestep"))
            sentence["f0_timestep"] = pitchIt->second["f0_timestep"];
    }

    DsDocument doc;
    doc.addRawSentence(sentence.dump());

    auto result = doc.saveFile(outputPath);
    if (!result.ok())
        return Result<void>::Error(result.error());

    return Result<void>::Ok();
}

} // namespace dstools
