#pragma once

#include <dsfw/TaskTypes.h>
#include <dstools/Result.h>

#include <QString>

#include <map>

namespace dstools {

class IFormatAdapter {
public:
    virtual ~IFormatAdapter() = default;
    virtual QString formatId() const = 0;
    virtual QString displayName() const = 0;
    virtual bool canImport() const { return false; }
    virtual bool canExport() const { return false; }

    virtual Result<void> importToLayers(
        const QString &filePath,
        std::map<QString, nlohmann::json> &layers,
        const ProcessorConfig &config) = 0;

    virtual Result<void> exportFromLayers(
        const std::map<QString, nlohmann::json> &layers,
        const QString &outputPath,
        const ProcessorConfig &config) = 0;
};

} // namespace dstools
