#include "DsFileAdapter.h"

#include <dstools/DsDocument.h>
#include <dstools/TimePos.h>

namespace dstools {

Result<void> DsFileAdapter::importToLayers(const QString &filePath,
                                           std::map<QString, nlohmann::json> &layers,
                                           const ProcessorConfig &config) {
    auto docResult = DsDocument::loadFile(filePath);
    if (!docResult.ok())
        return Result<void>::Error(docResult.error());

    auto &doc = docResult.value();
    if (doc.isEmpty())
        return Result<void>::Error("DS file contains no sentences");

    // Select sentence by index if specified
    int idx = 0;
    if (config.contains("index"))
        idx = config["index"].get<int>();
    if (idx < 0 || idx >= doc.sentenceCount())
        return Result<void>::Error("Sentence index out of range");

    const auto &s = doc.sentence(idx);

    // offset
    const double offset = DsDocument::numberOrString(s, "offset", 0.0);

    // ph_seq + ph_dur → phoneme layer
    if (s.contains("ph_seq") && s.contains("ph_dur")) {
        const auto phSeq = QString::fromStdString(s["ph_seq"].get<std::string>());
        const auto phDur = QString::fromStdString(s["ph_dur"].get<std::string>());
        const QStringList phones = phSeq.split(' ', Qt::SkipEmptyParts);
        const QStringList durs = phDur.split(' ', Qt::SkipEmptyParts);

        nlohmann::json boundaries = nlohmann::json::array();
        TimePos cumPos = secToUs(offset);
        const int count = qMin(phones.size(), durs.size());
        for (int i = 0; i < count; ++i) {
            boundaries.push_back(
                {{"id", i + 1}, {"pos", cumPos}, {"text", phones[i].toStdString()}});
            cumPos += secToUs(durs[i].toDouble());
        }
        boundaries.push_back({{"id", count + 1}, {"pos", cumPos}, {"text", ""}});

        layers[QStringLiteral("phoneme")] = {{"name", "phoneme"}, {"boundaries", boundaries}};
    }

    // note_seq + note_dur → midi layer
    if (s.contains("note_seq") && s.contains("note_dur")) {
        const auto noteSeq = QString::fromStdString(s["note_seq"].get<std::string>());
        const auto noteDur = QString::fromStdString(s["note_dur"].get<std::string>());
        const QStringList notes = noteSeq.split(' ', Qt::SkipEmptyParts);
        const QStringList noteDurs = noteDur.split(' ', Qt::SkipEmptyParts);

        nlohmann::json boundaries = nlohmann::json::array();
        TimePos cumPos = secToUs(offset);
        const int count = qMin(notes.size(), noteDurs.size());
        for (int i = 0; i < count; ++i) {
            boundaries.push_back(
                {{"id", i + 1}, {"pos", cumPos}, {"text", notes[i].toStdString()}});
            cumPos += secToUs(noteDurs[i].toDouble());
        }
        boundaries.push_back({{"id", count + 1}, {"pos", cumPos}, {"text", ""}});

        layers[QStringLiteral("midi")] = {{"name", "midi"}, {"boundaries", boundaries}};
    }

    // f0_seq + f0_timestep → pitch curve layer
    if (s.contains("f0_seq") && s.contains("f0_timestep")) {
        const double timestep = DsDocument::numberOrString(s, "f0_timestep", 0.005);
        layers[QStringLiteral("pitch")] = {
            {"name", "pitch"},
            {"f0_seq", s["f0_seq"]},
            {"f0_timestep", timestep},
            {"offset", offset}};
    }

    return Result<void>::Ok();
}

Result<void> DsFileAdapter::exportFromLayers(const std::map<QString, nlohmann::json> &layers,
                                             const QString &outputPath,
                                             const ProcessorConfig &config) {
    nlohmann::json sentence;

    // offset
    const double offset =
        config.contains("offset") ? config["offset"].get<double>() : 0.0;
    sentence["offset"] = offset;

    // phoneme layer → ph_seq + ph_dur
    auto phIt = layers.find(QStringLiteral("phoneme"));
    if (phIt != layers.end()) {
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

    // midi layer → note_seq + note_dur
    auto midiIt = layers.find(QStringLiteral("midi"));
    if (midiIt != layers.end()) {
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

    // pitch curve
    auto pitchIt = layers.find(QStringLiteral("pitch"));
    if (pitchIt != layers.end()) {
        if (pitchIt->second.contains("f0_seq"))
            sentence["f0_seq"] = pitchIt->second["f0_seq"];
        if (pitchIt->second.contains("f0_timestep"))
            sentence["f0_timestep"] = pitchIt->second["f0_timestep"];
    }

    DsDocument doc;
    doc.sentences().push_back(sentence);

    auto result = doc.saveFile(outputPath);
    if (!result.ok())
        return Result<void>::Error(result.error());

    return Result<void>::Ok();
}

} // namespace dstools
