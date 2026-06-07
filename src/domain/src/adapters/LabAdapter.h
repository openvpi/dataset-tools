#pragma once

#include <dsfw/IFormatAdapter.h>

namespace dstools {

    using namespace dsfw;

    class LabAdapter : public IFormatAdapter {
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

        Result<void> importToLayers(const QString &filePath, std::map<QString, LayerData> &layers,
                                    const ProcessorConfig &config) override;

        Result<void> exportFromLayers(const std::map<QString, LayerData> &layers, const QString &outputPath,
                                      const ProcessorConfig &config) override;
    };

} // namespace dstools
