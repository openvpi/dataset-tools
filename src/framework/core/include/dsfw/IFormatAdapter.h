#pragma once

#include <QString>
#include <dsfw/TaskTypes.h>
#include <dstools/Result.h>
#include <map>

namespace dsfw {

    class IFormatAdapter {
    public:
        static constexpr int kInterfaceVersion = 1;

        virtual ~IFormatAdapter() noexcept = default;
        virtual QString formatId() const = 0;
        virtual QString displayName() const = 0;
        virtual int interfaceVersion() const noexcept {
            return kInterfaceVersion;
        }
        virtual bool canImport() const noexcept {
            return false;
        }
        virtual bool canExport() const noexcept {
            return false;
        }

        [[nodiscard]] virtual Result<void> importToLayers(const QString &filePath, std::map<QString, LayerData> &layers,
                                            const ProcessorConfig &config) = 0;

        [[nodiscard]] virtual Result<void> exportFromLayers(const std::map<QString, LayerData> &layers,
                                              const QString &outputPath, const ProcessorConfig &config) = 0;
    };

} // namespace dsfw

// Backward compatibility
namespace dstools {
    using dsfw::IFormatAdapter;
}
