#pragma once

#include <dsfw/IFormatAdapter.h>
#include <dsfw/PipelineContext.h>

#include <vector>

namespace dstools {

class CsvAdapter : public IFormatAdapter {
public:
    QString formatId() const override { return QStringLiteral("csv"); }
    QString displayName() const override { return QStringLiteral("Transcription CSV"); }
    bool canImport() const override { return true; }
    bool canExport() const override { return true; }

    Result<void> importToLayers(const QString &filePath,
                                std::map<QString, nlohmann::json> &layers,
                                const ProcessorConfig &config) override;

    Result<void> exportFromLayers(const std::map<QString, nlohmann::json> &layers,
                                  const QString &outputPath,
                                  const ProcessorConfig &config) override;

    static Result<void> batchExport(const std::vector<PipelineContext> &contexts,
                                    const QString &outputPath,
                                    const ProcessorConfig &config = {});
};

} // namespace dstools
