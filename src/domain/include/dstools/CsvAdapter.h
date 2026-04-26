#pragma once

#include <dsfw/IFormatAdapter.h>
#include <dsfw/PipelineContext.h>
#include <vector>

namespace dstools {

struct TranscriptionRow;

class CsvAdapter : public IFormatAdapter {
public:
    QString formatId() const override { return QStringLiteral("csv"); }
    QString displayName() const override { return QStringLiteral("Transcription CSV"); }
    bool canImport() const override { return true; }
    bool canExport() const override { return true; }

    Result<void> importToLayers(const QString &filePath, std::map<QString, LayerData> &layers,
                                const ProcessorConfig &config) override;

    Result<void> exportFromLayers(const std::map<QString, LayerData> &layers, const QString &outputPath,
                                  const ProcessorConfig &config) override;

    static Result<void> batchExport(const std::vector<PipelineContext> &contexts, const QString &outputPath,
                                    const ProcessorConfig &config = {});

    static Result<void> writeRows(const QString &outputPath, const std::vector<TranscriptionRow> &rows);

    static Result<void> readRows(const QString &filePath, std::vector<TranscriptionRow> &rows);
};

} // namespace dstools