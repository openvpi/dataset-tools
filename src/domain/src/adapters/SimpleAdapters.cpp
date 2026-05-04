#include <dstools/DomainInit.h>
#include <dsfw/FormatAdapterRegistry.h>
#include <dsfw/PipelineContext.h>
#include "CsvAdapter.h"
#include "DsFileAdapter.h"
#include "LabAdapter.h"
#include "TextGridAdapter.h"

#include <dstools/TextGridToCsv.h>
#include <dstools/TimePos.h>

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

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
                                             std::map<QString, nlohmann::json> &layers,
                                             const ProcessorConfig & /*config*/) {
    TranscriptionRow row;
    QString error;
    if (!TextGridToCsv::extractFromTextGrid(filePath, row, error))
        return Result<void>::Error(error.toStdString());

    // Convert phSeq + phDur → phoneme layer boundaries
    const QStringList phones = row.phSeq.split(' ', Qt::SkipEmptyParts);
    const QStringList durs = row.phDur.split(' ', Qt::SkipEmptyParts);

    if (phones.size() != durs.size())
        return Result<void>::Error("ph_seq and ph_dur count mismatch");

    nlohmann::json boundaries = nlohmann::json::array();
    TimePos cumPos = 0;
    for (int i = 0; i < phones.size(); ++i) {
        boundaries.push_back({{"id", i + 1}, {"pos", cumPos}, {"text", phones[i].toStdString()}});
        cumPos += secToUs(durs[i].toDouble());
    }
    // Final boundary
    boundaries.push_back({{"id", phones.size() + 1}, {"pos", cumPos}, {"text", ""}});

    layers[QStringLiteral("phoneme")] = {{"name", "phoneme"}, {"boundaries", boundaries}};

    // ph_num layer
    if (!row.phNum.isEmpty()) {
        const QStringList nums = row.phNum.split(' ', Qt::SkipEmptyParts);
        nlohmann::json phNumArr = nlohmann::json::array();
        for (const auto &n : nums)
            phNumArr.push_back(n.toInt());
        layers[QStringLiteral("ph_num")] = phNumArr;
    }

    return Result<void>::Ok();
}

Result<void> TextGridAdapter::exportFromLayers(const std::map<QString, nlohmann::json> & /*layers*/,
                                               const QString & /*outputPath*/,
                                               const ProcessorConfig & /*config*/) {
    return Result<void>::Error("TextGrid export not implemented");
}

// ─── LabAdapter ───────────────────────────────────────────────────────────────

Result<void> LabAdapter::importToLayers(const QString &filePath,
                                        std::map<QString, nlohmann::json> &layers,
                                        const ProcessorConfig & /*config*/) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return Result<void>::Error("Cannot open file: " + filePath.toStdString());

    const QString content = QTextStream(&file).readAll().trimmed();
    file.close();

    const QStringList syllables = content.split(QRegularExpression(R"(\s+)"), Qt::SkipEmptyParts);

    nlohmann::json boundaries = nlohmann::json::array();
    int id = 1;
    for (const auto &syl : syllables) {
        boundaries.push_back({{"id", id}, {"pos", 0}, {"text", syl.toStdString()}});
        ++id;
    }
    // Final boundary with empty text
    boundaries.push_back({{"id", id}, {"pos", 0}, {"text", ""}});

    layers[QStringLiteral("grapheme")] = {{"name", "grapheme"}, {"boundaries", boundaries}};
    return Result<void>::Ok();
}

Result<void> LabAdapter::exportFromLayers(const std::map<QString, nlohmann::json> &layers,
                                          const QString &outputPath,
                                          const ProcessorConfig & /*config*/) {
    auto it = layers.find(QStringLiteral("grapheme"));
    if (it == layers.end())
        return Result<void>::Error("No grapheme layer found");

    const auto &boundaries = it->second["boundaries"];
    QStringList texts;
    for (const auto &b : boundaries) {
        const auto text = QString::fromStdString(b.value("text", ""));
        if (!text.isEmpty())
            texts.append(text);
    }

    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return Result<void>::Error("Cannot write file: " + outputPath.toStdString());

    QTextStream out(&file);
    out << texts.join(' ');
    file.close();
    return Result<void>::Ok();
}

} // namespace dstools
