#pragma once

#include <dsfw/Result.h>
#include <filesystem>
#include <string>
#include <vector>

namespace dstools {

class ProjectBackupManager {
public:
    static constexpr int kDefaultKeepCount = 10;

    static dsfw::Result<void> createBackup(const std::filesystem::path& projectPath);

    static dsfw::Result<std::vector<std::filesystem::path>> listBackups(const std::filesystem::path& projectPath);

    static dsfw::Result<void> restoreFromBackup(const std::filesystem::path& backupPath,
                                                const std::filesystem::path& targetPath);

    static dsfw::Result<void> pruneBackups(const std::filesystem::path& projectPath, int keepCount = kDefaultKeepCount);

    static dsfw::Result<std::filesystem::path> findLatestBackup(const std::filesystem::path& projectPath);

    static std::filesystem::path backupDir(const std::filesystem::path& projectPath);

    static std::filesystem::path backupFileName(const std::filesystem::path& projectPath);

private:
    static std::string makeTimestamp();
};

}  // namespace dstools