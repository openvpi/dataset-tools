#pragma once

#include <dsfw/IFormatAdapter.h>

namespace dstools {

    class LabAdapter : public dsfw::IFormatAdapter {
    public:
        QString formatId() const override {
            return QStringLiteral("lab");
        }
        QString displayName() const override {
            return QStringLiteral("Lab (syllables)");
        }
        bool canImport() const noexcept override {
            return true;
        }
        bool canExport() const noexcept override {
            return true;
        }

        dsfw::Result<void> importToLayers(const QString &filePath, std::map<QString, dsfw::LayerData> &layers,
                                    const dsfw::ProcessorConfig &config) override;

        dsfw::Result<void> exportFromLayers(const std::map<QString, dsfw::LayerData> &layers, const QString &outputPath,
                                      const dsfw::ProcessorConfig &config) override;
    };

} // namespace dstools
