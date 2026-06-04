#pragma once

#include <dsfw/IFormatAdapter.h>

namespace dstools {

    class TextGridAdapter : public IFormatAdapter {
    public:
        QString formatId() const override {
            return QStringLiteral("textgrid");
        }
        QString displayName() const override {
            return QStringLiteral("Praat TextGrid");
        }
        bool canImport() const noexcept override {
            return true;
        }
        bool canExport() const noexcept override {
            return false;
        }

        Result<void> importToLayers(const QString &filePath, std::map<QString, LayerData> &layers,
                                    const ProcessorConfig &config) override;

        Result<void> exportFromLayers(const std::map<QString, LayerData> &layers, const QString &outputPath,
                                      const ProcessorConfig &config) override;
    };

} // namespace dstools
