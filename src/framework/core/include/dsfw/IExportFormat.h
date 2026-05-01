#pragma once

#include <dsfw/ExportTypes.h>
#include <dstools/Result.h>

#include <filesystem>
#include <string>

namespace dstools {

class IExportFormat {
public:
    virtual ~IExportFormat() = default;

    virtual const char *formatName() const = 0;
    virtual const char *formatExtension() const = 0;

    virtual Result<void> exportItem(const std::filesystem::path &sourceFile,
                                    const std::filesystem::path &workingDir,
                                    const std::filesystem::path &outputPath) = 0;

    virtual Result<void> exportAll(const std::filesystem::path &workingDir,
                                   const std::filesystem::path &outputDir) = 0;
};

} // namespace dstools
