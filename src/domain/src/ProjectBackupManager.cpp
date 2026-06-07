#include <dstools/ProjectBackupManager.h>

#include <dsfw/AtomicFileWriter.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <sstream>

namespace dstools {

std::string ProjectBackupManager::makeTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm_buf{};
#ifdef _WIN32
    if (localtime_s(&tm_buf, &time) != 0) {
        return "00000000_000000_000";
    }
#else
    localtime_r(&time, &tm_buf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y%m%d_%H%M%S") << '_' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::filesystem::path ProjectBackupManager::backupDir(const std::filesystem::path& projectPath) {
    return projectPath.parent_path() / ".backups";
}

std::filesystem::path ProjectBackupManager::backupFileName(const std::filesystem::path& projectPath) {
    auto stem = projectPath.stem().u8string();
    auto ts = makeTimestamp();
    return backupDir(projectPath) / (std::string(stem.begin(), stem.end()) + "_" + ts + ".bak");
}

dsfw::Result<void> ProjectBackupManager::createBackup(const std::filesystem::path& projectPath) {
    std::error_code ec;
    if (!std::filesystem::exists(projectPath, ec)) {
        return dsfw::Err("Project file does not exist: " +
                         std::string(projectPath.u8string().begin(), projectPath.u8string().end()));
    }

    auto dir = backupDir(projectPath);
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        return dsfw::Err("Cannot create backup directory: " +
                         std::string(dir.u8string().begin(), dir.u8string().end()));
    }

    std::ifstream src(projectPath, std::ios::binary);
    if (!src.is_open()) {
        return dsfw::Err("Cannot open project file for backup: " +
                         std::string(projectPath.u8string().begin(), projectPath.u8string().end()));
    }

    std::string content((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());
    src.close();

    auto bakPath = backupFileName(projectPath);
    return dsfw::AtomicFileWriter::write(bakPath, content);
}

dsfw::Result<std::vector<std::filesystem::path>>
ProjectBackupManager::listBackups(const std::filesystem::path& projectPath) {
    auto dir = backupDir(projectPath);
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec)) {
        return std::vector<std::filesystem::path>{};
    }

    std::vector<std::filesystem::path> backups;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) {
            break;
        }
        if (entry.is_regular_file() && entry.path().extension() == ".bak") {
            backups.push_back(entry.path());
        }
    }

    std::sort(backups.begin(), backups.end(), std::greater<>());
    return backups;
}

dsfw::Result<std::filesystem::path> ProjectBackupManager::findLatestBackup(const std::filesystem::path& projectPath) {
    auto result = listBackups(projectPath);
    if (!result) {
        return dsfw::Err<std::filesystem::path>(result.error());
    }

    const auto& backups = result.value();
    if (backups.empty()) {
        return std::filesystem::path{};
    }

    return backups.front();
}

dsfw::Result<void> ProjectBackupManager::restoreFromBackup(const std::filesystem::path& backupPath,
                                                           const std::filesystem::path& targetPath) {
    std::error_code ec;
    if (!std::filesystem::exists(backupPath, ec)) {
        return dsfw::Err("Backup file does not exist: " +
                         std::string(backupPath.u8string().begin(), backupPath.u8string().end()));
    }

    std::ifstream src(backupPath, std::ios::binary);
    if (!src.is_open()) {
        return dsfw::Err("Cannot open backup file: " +
                         std::string(backupPath.u8string().begin(), backupPath.u8string().end()));
    }

    std::string content((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());
    src.close();

    return dsfw::AtomicFileWriter::write(targetPath, content);
}

dsfw::Result<void> ProjectBackupManager::pruneBackups(const std::filesystem::path& projectPath, int keepCount) {
    auto result = listBackups(projectPath);
    if (!result) {
        return dsfw::Err(result.error());
    }

    auto& backups = result.value();
    if (static_cast<int>(backups.size()) <= keepCount) {
        return dsfw::Ok();
    }

    std::error_code ec;
    for (size_t i = static_cast<size_t>(keepCount); i < backups.size(); ++i) {
        std::filesystem::remove(backups[i], ec);
    }

    return dsfw::Ok();
}

}  // namespace dstools