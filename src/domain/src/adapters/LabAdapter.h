#pragma once

#include <dsfw/IFormatAdapter.h>

namespace dstools {

class LabAdapter : public IFormatAdapter {
public:
    QString formatId() const override { return QStringLiteral("lab"); }
    QString displayName() const override { return QStringLiteral("Lab (syllables)"); }
    bool canImport() const override { return true; }
    bool canExport() const override { return true; }

    Result<void> importToLayers(const QString &filePath,
                                std::map<QString, nlohmann::json> &layers,
                                const ProcessorConfig &config) override;

    Result<void> exportFromLayers(const std::map<QString, nlohmann::json> &layers,
                                  const QString &outputPath,
                                  const ProcessorConfig &config) override;
};

} // namespace dstools
