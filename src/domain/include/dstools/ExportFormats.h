#pragma once

#include <dsfw/IExportFormat.h>

namespace dstools {

class HtsLabelExportFormat : public IExportFormat {
public:
    const char *formatName() const override;
    const char *formatExtension() const override;

    Result<void> exportItem(const std::filesystem::path &sourceFile,
                            const std::filesystem::path &workingDir,
                            const std::filesystem::path &outputPath) override;

    Result<void> exportAll(const std::filesystem::path &workingDir,
                           const std::filesystem::path &outputDir) override;
};

class SinsyXmlExportFormat : public IExportFormat {
public:
    const char *formatName() const override;
    const char *formatExtension() const override;

    Result<void> exportItem(const std::filesystem::path &sourceFile,
                            const std::filesystem::path &workingDir,
                            const std::filesystem::path &outputPath) override;

    Result<void> exportAll(const std::filesystem::path &workingDir,
                           const std::filesystem::path &outputDir) override;
};

} // namespace dstools
