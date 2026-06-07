#pragma once

#include <dsfw/IFormatAdapter.h>
#include <dsfw/PipelineContext.h>
#include <vector>

namespace dstools {

struct TranscriptionRow;

class CsvAdapter : public dsfw::IFormatAdapter {
public:
    QString formatId() const override { return QStringLiteral("csv"); }
    QString displayName() const override { return QStringLiteral("Transcription CSV"); }
    bool canImport() const noexcept override { return true; }
    bool canExport() const noexcept override { return true; }

    dsfw::Result<void> importToLayers(const QString& filePath, std::map<QString, dsfw::LayerData>& layers,
                                const dsfw::ProcessorConfig& config) override;

    dsfw::Result<void> exportFromLayers(const std::map<QString, dsfw::LayerData>& layers, const QString& outputPath,
                                  const dsfw::ProcessorConfig& config) override;

    static dsfw::Result<void> batchExport(const std::vector<dsfw::PipelineContext>& contexts, const QString& outputPath,
                                    const dsfw::ProcessorConfig& config = {});

    static dsfw::Result<void> writeRows(const QString& outputPath, const std::vector<TranscriptionRow>& rows);

    static dsfw::Result<void> readRows(const QString& filePath, std::vector<TranscriptionRow>& rows);
};

} // namespace dstools