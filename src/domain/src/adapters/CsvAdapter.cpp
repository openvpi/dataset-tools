#include "CsvAdapter.h"

#include <dstools/TranscriptionCsv.h>
#include <dstools/TimePos.h>

namespace dstools {

static void phonemeRowToLayers(const TranscriptionRow &row,
                               std::map<QString, nlohmann::json> &layers) {
    const QStringList phones = row.phSeq.split(' ', Qt::SkipEmptyParts);
    const QStringList durs = row.phDur.split(' ', Qt::SkipEmptyParts);

    nlohmann::json boundaries = nlohmann::json::array();
    TimePos cumPos = 0;
    const int count = qMin(phones.size(), durs.size());
    for (int i = 0; i < count; ++i) {
        boundaries.push_back({{"id", i + 1}, {"pos", cumPos}, {"text", phones[i].toStdString()}});
        cumPos += secToUs(durs[i].toDouble());
    }
    boundaries.push_back({{"id", count + 1}, {"pos", cumPos}, {"text", ""}});

    layers[QStringLiteral("phoneme")] = {{"name", "phoneme"}, {"boundaries", boundaries}};

    if (!row.phNum.isEmpty()) {
        const QStringList nums = row.phNum.split(' ', Qt::SkipEmptyParts);
        nlohmann::json phNumArr = nlohmann::json::array();
        for (const auto &n : nums)
            phNumArr.push_back(n.toInt());
        layers[QStringLiteral("ph_num")] = phNumArr;
    }
}

Result<void> CsvAdapter::importToLayers(const QString &filePath,
                                        std::map<QString, nlohmann::json> &layers,
                                        const ProcessorConfig &config) {
    std::vector<TranscriptionRow> rows;
    QString error;
    if (!TranscriptionCsv::read(filePath, rows, error))
        return Result<void>::Error(error.toStdString());

    if (rows.empty())
        return Result<void>::Error("CSV file contains no rows");

    // Find matching row by name if specified, otherwise use first row
    const TranscriptionRow *target = &rows[0];
    if (config.contains("name")) {
        const auto name = QString::fromStdString(config["name"].get<std::string>());
        for (const auto &r : rows) {
            if (r.name == name) {
                target = &r;
                break;
            }
        }
    }

    phonemeRowToLayers(*target, layers);
    return Result<void>::Ok();
}

Result<void> CsvAdapter::exportFromLayers(const std::map<QString, nlohmann::json> &layers,
                                          const QString &outputPath,
                                          const ProcessorConfig &config) {
    auto it = layers.find(QStringLiteral("phoneme"));
    if (it == layers.end())
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
    row.name = config.contains("name")
                   ? QString::fromStdString(config["name"].get<std::string>())
                   : QStringLiteral("item");
    row.phSeq = phones.join(' ');
    row.phDur = durs.join(' ');

    // ph_num
    auto pnIt = layers.find(QStringLiteral("ph_num"));
    if (pnIt != layers.end() && pnIt->second.is_array()) {
        QStringList nums;
        for (const auto &v : pnIt->second)
            nums.append(QString::number(v.get<int>()));
        row.phNum = nums.join(' ');
    }

    QString error;
    if (!TranscriptionCsv::write(outputPath, {row}, error))
        return Result<void>::Error(error.toStdString());

    return Result<void>::Ok();
}

Result<void> CsvAdapter::batchExport(const std::vector<PipelineContext> &contexts,
                                      const QString &outputPath,
                                      const ProcessorConfig &config) {
    std::vector<TranscriptionRow> rows;

    for (const auto &ctx : contexts) {
        if (ctx.status != PipelineContext::Status::Active)
            continue;

        auto it = ctx.layers.find(QStringLiteral("phoneme"));
        if (it == ctx.layers.end())
            continue;

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
        row.name = ctx.itemId;
        row.phSeq = phones.join(' ');
        row.phDur = durs.join(' ');

        auto pnIt = ctx.layers.find(QStringLiteral("ph_num"));
        if (pnIt != ctx.layers.end() && pnIt->second.is_array()) {
            QStringList nums;
            for (const auto &v : pnIt->second)
                nums.append(QString::number(v.get<int>()));
            row.phNum = nums.join(' ');
        }

        rows.push_back(std::move(row));
    }

    if (rows.empty())
        return Result<void>::Error("No valid items to export");

    QString error;
    if (!TranscriptionCsv::write(outputPath, rows, error))
        return Result<void>::Error(error.toStdString());

    return Result<void>::Ok();
}

} // namespace dstools
