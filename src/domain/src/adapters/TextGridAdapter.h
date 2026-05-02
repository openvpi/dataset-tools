#pragma once

#include <dsfw/IFormatAdapter.h>

namespace dstools {

class TextGridAdapter : public IFormatAdapter {
public:
    QString formatId() const override { return QStringLiteral("textgrid"); }
    QString displayName() const override { return QStringLiteral("Praat TextGrid"); }
    bool canImport() const override { return true; }
    bool canExport() const override { return false; }

    Result<void> importToLayers(const QString &filePath,
                                std::map<QString, nlohmann::json> &layers,
                                const ProcessorConfig &config) override;

    Result<void> exportFromLayers(const std::map<QString, nlohmann::json> &layers,
                                  const QString &outputPath,
                                  const ProcessorConfig &config) override;
};

} // namespace dstools
