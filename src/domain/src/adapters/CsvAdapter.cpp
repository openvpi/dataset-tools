#include <dstools/CsvAdapter.h>
#include "PhonemeLayerBuilder.h"

#include <dsfw/ConfigTypes.h>
#include <dstools/TranscriptionCsv.h>
#include <dstools/TimePos.h>

namespace dstools {

static std::map<QString, LayerData> phonemeRowToLayers(const TranscriptionRow &row) {
    std::map<QString, LayerData> temp;
    const QStringList phones = row.phSeq.split(' ', Qt::SkipEmptyParts);
    const QStringList durs = row.phDur.split(' ', Qt::SkipEmptyParts);

    auto boundaries = buildBoundaries(phones, durs, 0);
    temp[QStringLiteral("phoneme")] = LayerData::fromJson({{"name", "phoneme"}, {"boundaries", boundaries.toJson()}});

    if (!row.phNum.isEmpty()) {
        const QStringList nums = row.phNum.split(' ', Qt::SkipEmptyParts);
        nlohmann::json phNumArr = nlohmann::json::array();
        for (const auto &n : nums)
            phNumArr.push_back(n.toInt());
        temp[QStringLiteral("ph_num")] = LayerData::fromJson(phNumArr);
    }

    return temp;
}

static std::map<QString, nlohmann::json> layersToJson(const std::map<QString, LayerData> &layers) {
    std::map<QString, nlohmann::json> result;
    for (const auto &[key, val] : layers)
        result[key] = val.toJson();
    return result;
}

static Result<void> validateLayerData(const std::map<QString, LayerData> &layers) {
    if (layers.empty())
        return Result<void>::Error("import produced no layers");
    for (const auto &[key, val] : layers) {
        if (val.empty())
            return Result<void>::Error(QString("layer '%1' is empty").arg(key));
        try {
            const auto j = val.toJson();
            if (j.is_null())
                return Result<void>::Error(QString("layer '%1' contains null JSON").arg(key));
        } catch (const std::exception &e) {
            return Result<void>::Error(QString("layer '%1' JSON parse error: %2").arg(key, e.what()));
        }
    }
    return Result<void>::Ok();
}

Result<void> CsvAdapter::importToLayers(const QString &filePath,
                                        std::map<QString, LayerData> &layers,
                                        const ProcessorConfig &config) {
    auto csvResult = TranscriptionCsv::read(filePath);
    if (!csvResult.ok())
        return Result<void>::Error(csvResult.error());

    auto rows = std::move(csvResult.value());
    if (rows.empty())
        return Result<void>::Error("CSV file contains no rows");

    const TranscriptionRow *target = &rows[0];
    if (config.contains(QStringLiteral("name"))) {
        const auto name = configValueString(config, QStringLiteral("name"));
        for (const auto &r : rows) {
            if (r.name == name) {
                target = &r;
                break;
            }
        }
    }

    auto temp = phonemeRowToLayers(*target);
    auto validation = validateLayerData(temp);
    if (!validation.ok())
        return validation;

    layers = std::move(temp);
    return Result<void>::Ok();
}

Result<void> CsvAdapter::exportFromLayers(const std::map<QString, LayerData> &layers,
                                          const QString &outputPath,
                                          const ProcessorConfig &config) {
    const auto lyr = layersToJson(layers);
    auto it = lyr.find(QStringLiteral("phoneme"));
    if (it == lyr.end())
        return Result<void>::Error("No phoneme layer found");

    const auto &boundaries = it->second["boundaries"];

    QStringList phones;
    QStringList durs;
    for (size_t i = 0; i + 1 < boundaries.size(); ++i) {
        phones.append(QString::fromStdString(boundaries[i].value("text", "")));
        const TimePos posA = boundaries[i].value("pos", int64_t(0));
        const TimePos posB = boundaries[i + 1].value("pos", int64_t(0));
        durs.append(QString::number(usToSec(posB - posA), 'f', 6));
    }

    TranscriptionRow row;
    row.name = config.contains(QStringLiteral("name"))
                   ? configValueString(config, QStringLiteral("name"))
                   : QStringLiteral("item");
    row.phSeq = phones.join(' ');
    row.phDur = durs.join(' ');

    auto pnIt = lyr.find(QStringLiteral("ph_num"));
    if (pnIt != lyr.end() && pnIt->second.is_array()) {
        QStringList nums;
        for (const auto &v : pnIt->second)
            nums.append(QString::number(v.get<int>()));
        row.phNum = nums.join(' ');
    }

    auto writeResult = TranscriptionCsv::write(outputPath, {row});
    if (!writeResult.ok())
        return Result<void>::Error(writeResult.error());

    return Result<void>::Ok();
}

Result<void> CsvAdapter::batchExport(const std::vector<PipelineContext> &contexts,
                                      const QString &outputPath,
                                      const ProcessorConfig & /*config*/) {
    std::vector<TranscriptionRow> rows;

    for (const auto &ctx : contexts) {
        if (ctx.status != PipelineContext::Status::Active)
            continue;

        auto it = ctx.layers.find(QStringLiteral("phoneme"));
        if (it == ctx.layers.end())
            continue;

        const nlohmann::json layerJson = it->second.toJson();
        const auto &boundaries = layerJson["boundaries"];

        QStringList phones;
        QStringList durs;
        for (size_t i = 0; i + 1 < boundaries.size(); ++i) {
            phones.append(QString::fromStdString(boundaries[i].value("text", "")));
            const TimePos posA = boundaries[i].value("pos", int64_t(0));
            const TimePos posB = boundaries[i + 1].value("pos", int64_t(0));
            durs.append(QString::number(usToSec(posB - posA), 'f', 6));
        }

        TranscriptionRow row;
        row.name = ctx.itemId;
        row.phSeq = phones.join(' ');
        row.phDur = durs.join(' ');

        auto pnIt = ctx.layers.find(QStringLiteral("ph_num"));
        if (pnIt != ctx.layers.end()) {
            const nlohmann::json pnJson = pnIt->second.toJson();
            if (pnJson.is_array()) {
                QStringList nums;
                for (const auto &v : pnJson)
                    nums.append(QString::number(v.get<int>()));
                row.phNum = nums.join(' ');
            }
        }

        rows.push_back(std::move(row));
    }

    if (rows.empty())
        return Result<void>::Error("No valid items to export");

    auto writeResult = TranscriptionCsv::write(outputPath, rows);
    if (!writeResult.ok())
        return Result<void>::Error(writeResult.error());

    return Result<void>::Ok();
}

Result<void> CsvAdapter::writeRows(const QString &outputPath, const std::vector<TranscriptionRow> &rows) {
    if (rows.empty())
        return Result<void>::Error("No rows to write");

    auto writeResult = TranscriptionCsv::write(outputPath, rows);
    if (!writeResult.ok())
        return Result<void>::Error(writeResult.error());

    return Result<void>::Ok();
}

Result<void> CsvAdapter::readRows(const QString &filePath, std::vector<TranscriptionRow> &rows) {
    auto csvResult = TranscriptionCsv::read(filePath);
    if (!csvResult.ok())
        return Result<void>::Error(csvResult.error());

    rows = std::move(csvResult.value());
    if (rows.empty())
        return Result<void>::Error("CSV file contains no rows");

    return Result<void>::Ok();
}

} // namespace dstools
