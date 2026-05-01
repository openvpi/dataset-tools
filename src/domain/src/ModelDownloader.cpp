#include <dstools/ModelDownloader.h>

#include <fstream>
#include <filesystem>

namespace dstools {

    Result<void> ModelDownloader::startDownload(const std::string &modelId, const std::string &url,
                                                 const std::filesystem::path &destPath,
                                                 const std::function<void(int)> &progress) {
        (void)modelId;
        (void)url;
        (void)destPath;
        (void)progress;
        return Err("Download not implemented");
    }

    Result<void> ModelDownloader::verifyChecksum(const std::filesystem::path &filePath,
                                                   const std::string &expectedHash) {
        (void)filePath;
        (void)expectedHash;
        return Err("Checksum verification not implemented");
    }

} // namespace dstools
