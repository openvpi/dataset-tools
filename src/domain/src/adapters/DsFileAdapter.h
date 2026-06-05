#pragma once

#include <dsfw/IFormatAdapter.h>

namespace dstools {

class DsFileAdapter : public dsfw::IFormatAdapter {
public:
    QString formatId() const override { return QStringLiteral("ds"); }
    QString displayName() const override { return QStringLiteral("DiffSinger DS"); }
    bool canImport() const noexcept override { return true; }
    bool canExport() const noexcept override { return true; }

    Result<void> importToLayers(const QString& filePath, std::map<QString, LayerData>& layers,
                                const ProcessorConfig& config) override;

    Result<void> exportFromLayers(const std::map<QString, LayerData>& layers, const QString& outputPath,
                                  const ProcessorConfig& config) override;
};

} // namespace dstools
