#pragma once

#include <atomic>
#include <dstools/Result.h>
#include <filesystem>
#include <string>

namespace dsfw {

    class AtomicFileWriter {
    public:
        /// Write content to path atomically (via QSaveFile).
        /// If backup is enabled (default), renames existing path → path.bak before writing.
        /// If validation is enabled (default), re-reads the file after writing and validates it parses correctly.
        static dstools::Result<void> write(const std::filesystem::path &path, const std::string &content);

        /// Same as write() but also validates content as JSON after writing.
        /// Falls back to backup file if validation fails.
        static dstools::Result<void> writeJson(const std::filesystem::path &path, const std::string &jsonContent);

        static void setBackupEnabled(bool enabled);
        static bool isBackupEnabled();

        static void setValidationEnabled(bool enabled);
        static bool isValidationEnabled();

    private:
        static dstools::Result<void> writeImpl(const std::filesystem::path &path,
                                               const std::string &content,
                                               bool validateJson);

        static std::atomic<bool> s_backupEnabled;
        static std::atomic<bool> s_validationEnabled;
    };

} // namespace dsfw