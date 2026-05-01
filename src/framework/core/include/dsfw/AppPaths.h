#pragma once
/// @file AppPaths.h
/// @brief Standard application directory paths (config, log, cache, data).

#include <QString>

namespace dsfw {

/// @brief Provides standard application directory paths.
class AppPaths {
public:
    /// @brief Get the configuration directory path.
    /// @return Path to the config directory.
    static QString configDir();

    /// @brief Get the log directory path.
    /// @return Path to the log directory.
    static QString logDir();

    /// @brief Get the crash dump directory path.
    /// @return Path to the dump directory.
    static QString dumpDir();

    /// @brief Get the cache directory path.
    /// @return Path to the cache directory.
    static QString cacheDir();

    /// @brief Get the data directory path.
    /// @return Path to the data directory.
    static QString dataDir();

    /// @brief Migrate files from legacy directory locations.
    static void migrateFromLegacyPaths();

private:
    static QString ensureDir(const QString &path);
};

} // namespace dsfw
