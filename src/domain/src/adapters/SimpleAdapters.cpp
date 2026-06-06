#include <dstools/DomainInit.h>
#include <dsfw/FormatAdapterRegistry.h>
#include <dsfw/PipelineContext.h>
#include <dstools/CsvAdapter.h>
#include "DsFileAdapter.h"
#include "LabAdapter.h"
#include "TextGridAdapter.h"
#include "PhonemeLayerBuilder.h"

#include <dstools/TextGridToCsv.h>
#include <dsfw/TimePos.h>

#include <dsfw/PathUtils.h>

#include <QRegularExpression>

// ─── Self-registration ────────────────────────────────────────────────────────

REGISTER_FORMAT_ADAPTER(dstools::TextGridAdapter);
REGISTER_FORMAT_ADAPTER(dstools::LabAdapter);

namespace dstools {

// ─── FormatAdapterInit ────────────────────────────────────────────────────────

void registerDomainFormatAdapters() {
    // Adapters are now self-registered via REGISTER_FORMAT_ADAPTER macro.
    // This function is kept for backward compatibility.
}

Result<void> exportContextsToCsv(const std::vector<PipelineContext>& contexts, const QString& outputPath,
                                 const ProcessorConfig& config) {
    return CsvAdapter::batchExport(contexts, outputPath, config);
}

// ─── Import validation helper ─────────────────────────────────────────────────

static Result<void> validateLayerData(const std::map<QString, LayerData>& layers) {
    if (layers.empty())
        return Result<void>::Error("import produced no layers");
    for (const auto& [key, val] : layers) {
        if (val.empty())
            return Result<void>::Error(QString("layer '%1' is empty").arg(key).toStdString());
        try {
            const auto j = val.toJson();
            if (j.is_null())
                return Result<void>::Error(QString("layer '%1' contains null JSON").arg(key).toStdString());
        } catch (const std::exception& e) {
            return Result<void>::Error(QString("layer '%1' JSON parse error: %2").arg(key, e.what()).toStdString());
        }
    }
    return Result<void>::Ok();
}

// ─── TextGridAdapter ──────────────────────────────────────────────────────────

Result<void> TextGridAdapter::importToLayers(const QString& filePath, std::map<QString, LayerData>& layers,
                                             const ProcessorConfig& /*config*/) {
    auto rowResult = TextGridToCsv::extractFromTextGrid(filePath);
    if (!rowResult.ok())
        return Result<void>::Error(rowResult.error());

    const auto& row = rowResult.value();

    const QStringList phones = row.phSeq.split(' ', Qt::SkipEmptyParts);
    const QStringList durs = row.phDur.split(' ', Qt::SkipEmptyParts);

    if (phones.size() != durs.size())
        return Result<void>::Error("ph_seq and ph_dur count mismatch");

    std::map<QString, LayerData> temp;
    auto boundaries = buildBoundaries(phones, durs, 0);
    temp[QStringLiteral("phoneme")] = LayerData::fromJson({{"name", "phoneme"}, {"boundaries", boundaries.toJson()}});

    if (!row.phNum.isEmpty()) {
        const QStringList nums = row.phNum.split(' ', Qt::SkipEmptyParts);
        nlohmann::json phNumArr = nlohmann::json::array();
        for (const auto& n : nums)
            phNumArr.push_back(n.toInt());
        temp[QStringLiteral("ph_num")] = LayerData::fromJson(phNumArr);
    }

    auto validation = validateLayerData(temp);
    if (!validation.ok())
        return validation;

    layers = std::move(temp);
    return Result<void>::Ok();
}

Result<void> TextGridAdapter::exportFromLayers(const std::map<QString, LayerData>& /*layers*/,
                                               const QString& /*outputPath*/, const ProcessorConfig& /*config*/) {
    return Result<void>::Error("TextGrid export not implemented");
}

// ─── LabAdapter ───────────────────────────────────────────────────────────────

Result<void> LabAdapter::importToLayers(const QString& filePath, std::map<QString, LayerData>& layers,
                                        const ProcessorConfig& /*config*/) {
    auto textResult = dsfw::PathUtils::readFile(filePath);
    if (!textResult.ok())
        return Result<void>::Error(textResult.error());

    const QString content = textResult.value().trimmed();

    const QStringList syllables = content.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);

    nlohmann::json boundaries = nlohmann::json::array();
    int id = 1;
    for (const auto& syl : syllables) {
        boundaries.push_back({{"id", id}, {"pos", 0}, {"text", syl.toStdString()}});
        ++id;
    }
    boundaries.push_back({{"id", id}, {"pos", 0}, {"text", ""}});

    std::map<QString, LayerData> temp;
    nlohmann::json grapheme = {{"name", "grapheme"}, {"boundaries", boundaries}};
    temp[QStringLiteral("grapheme")] = LayerData::fromJson(grapheme);

    auto validation = validateLayerData(temp);
    if (!validation.ok())
        return validation;

    layers = std::move(temp);
    return Result<void>::Ok();
}

Result<void> LabAdapter::exportFromLayers(const std::map<QString, LayerData>& layers, const QString& outputPath,
                                          const ProcessorConfig& /*config*/) {
    auto it = layers.find(QStringLiteral("grapheme"));
    if (it == layers.end())
        return Result<void>::Error("No grapheme layer found");

    const nlohmann::json layerJson = it->second.toJson();
    const auto& boundaries = layerJson["boundaries"];
    QStringList texts;
    for (const auto& b : boundaries) {
        const auto text = QString::fromStdString(b.value("text", ""));
        if (!text.isEmpty())
            texts.append(text);
    }

    auto writeResult = dsfw::PathUtils::writeFile(outputPath, texts.join(' '));
    if (!writeResult.ok())
        return Result<void>::Error(writeResult.error());
    return Result<void>::Ok();
}

} // namespace dstools
