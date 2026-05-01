#pragma once

#include <dsfw/ModelTypes.h>
#include <dstools/Result.h>

#include <filesystem>
#include <functional>
#include <string>

namespace dstools {

class IModelDownloader {
public:
    virtual ~IModelDownloader() = default;

    virtual const char *downloaderName() const = 0;

    virtual Result<void> startDownload(const std::string &modelId, const std::string &url,
                                       const std::filesystem::path &destPath,
                                       const std::function<void(int)> &progress) = 0;

    virtual Result<void> verifyChecksum(const std::filesystem::path &filePath,
                                        const std::string &expectedHash) = 0;
};

} // namespace dstools
