#include <dstools/DomainInit.h>
#include <dsfw/FormatAdapterRegistry.h>
#include <dsfw/PipelineContext.h>
#include <dstools/CsvAdapter.h>
#include "DsFileAdapter.h"
#include "LabAdapter.h"
#include "TextGridAdapter.h"
#include "PhonemeLayerBuilder.h"

#include <dstools/TextGridToCsv.h>
#include <dstools/TimePos.h>

#include <dsfw/Encoding.h>

#include <QRegularExpression>

namespace dstools {

// ─── FormatAdapterInit ────────────────────────────────────────────────────────

void registerDomainFormatAdapters() {
    auto &registry = FormatAdapterRegistry::instance();
    registry.registerAdapter(std::make_unique<TextGridAdapter>());
    registry.registerAdapter(std::make_unique<CsvAdapter>());
    registry.registerAdapter(std::make_unique<DsFileAdapter>());
    registry.registerAdapter(std::make_unique<LabAdapter>());
}

Result<void> exportContextsToCsv(const std::vector<PipelineContext> &contexts,
                                  const QString &outputPath,
                                  const ProcessorConfig &config) {
    return CsvAdapter::batchExport(contexts, outputPath, config);
}

// ─── TextGridAdapter ──────────────────────────────────────────────────────────

Result<void> TextGridAdapter::importToLayers(const QString &filePath,
                                             std::map<QString, LayerData> &layers,
                                             const ProcessorConfig & /*config*/) {
    auto rowResult = TextGridToCsv::extractFromTextGrid(filePath);
    if (!rowResult.ok())
        return Result<void>::Error(rowResult.error());

    const auto &row = rowResult.value();

    const QStringList phones = row.phSeq.split(' ', Qt::SkipEmptyParts);
    const QStringList durs = row.phDur.split(' ', Qt::SkipEmptyParts);

    if (phones.size() != durs.size())
        return Result<void>::Error("ph_seq and ph_dur count mismatch");

    std::map<QString, nlohmann::json> internal;

    auto boundaries = buildBoundaries(phones, durs, 0);
    internal[QStringLiteral("phoneme")] = {{"name", "phoneme"}, {"boundaries", boundaries.toJson()}};

    if (!row.phNum.isEmpty()) {
        const QStringList nums = row.phNum.split(' ', Qt::SkipEmptyParts);
        nlohmann::json phNumArr = nlohmann::json::array();
        for (const auto &n : nums)
            phNumArr.push_back(n.toInt());
        internal[QStringLiteral("ph_num")] = phNumArr;
    }

    for (const auto &[key, val] : internal)
        layers[key] = LayerData::fromJson(val);

    return Result<void>::Ok();
}

Result<void> TextGridAdapter::exportFromLayers(const std::map<QString, LayerData> & /*layers*/,
                                               const QString & /*outputPath*/,
                                               const ProcessorConfig & /*config*/) {
    return Result<void>::Error("TextGrid export not implemented");
}

// ─── LabAdapter ───────────────────────────────────────────────────────────────

Result<void> LabAdapter::importToLayers(const QString &filePath,
                                        std::map<QString, LayerData> &layers,
                                        const ProcessorConfig & /*config*/) {
    auto textResult = dsfw::Encoding::readFile(filePath);
    if (!textResult.ok())
        return Result<void>::Error(textResult.error());

    const QString content = textResult.value().trimmed();

    const QStringList syllables = content.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);

    nlohmann::json boundaries = nlohmann::json::array();
    int id = 1;
    for (const auto &syl : syllables) {
        boundaries.push_back({{"id", id}, {"pos", 0}, {"text", syl.toStdString()}});
        ++id;
    }
    boundaries.push_back({{"id", id}, {"pos", 0}, {"text", ""}});

    nlohmann::json grapheme = {{"name", "grapheme"}, {"boundaries", boundaries}};
    layers[QStringLiteral("grapheme")] = LayerData::fromJson(grapheme);
    return Result<void>::Ok();
}

Result<void> LabAdapter::exportFromLayers(const std::map<QString, LayerData> &layers,
                                          const QString &outputPath,
                                          const ProcessorConfig & /*config*/) {
    auto it = layers.find(QStringLiteral("grapheme"));
    if (it == layers.end())
        return Result<void>::Error("No grapheme layer found");

    const nlohmann::json layerJson = it->second.toJson();
    const auto &boundaries = layerJson["boundaries"];
    QStringList texts;
    for (const auto &b : boundaries) {
        const auto text = QString::fromStdString(b.value("text", ""));
        if (!text.isEmpty())
            texts.append(text);
    }

    auto writeResult = dsfw::Encoding::writeFile(outputPath, texts.join(' '));
    if (!writeResult.ok())
        return Result<void>::Error(writeResult.error());
    return Result<void>::Ok();
}

} // namespace dstools
