#pragma once

/// @file ConfigMigration.h
/// @brief Version-based configuration migration infrastructure.
///
/// Usage:
///   ConfigMigration migrator;
///   migrator.registerMigration(1, [](nlohmann::json &j) {
///       // Migrate from version 0 to 1
///       return true;
///   });
///   migrator.migrate(json, currentVersion);

#include <dsfw/Result.h>

#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

namespace dstools {

/// @brief Handles version-based configuration migration.
///
/// Each migration function receives the JSON object and mutates it in-place.
/// Migrations are applied in version order (v0→v1, v1→v2, ...).
class ConfigMigration {
public:
    using MigrationFunc = std::function<bool(nlohmann::json&)>;

    /// Register a migration step from (targetVersion - 1) to targetVersion.
    void registerMigration(int targetVersion, MigrationFunc func);

    /// Migrate `json` from its current `configVersion` to the latest registered version.
    /// Returns Error() if any migration step fails.
    dsfw::Result<void> migrate(nlohmann::json& json, int currentConfigVersion) const;

private:
    std::unordered_map<int, MigrationFunc> m_migrations;
};

}  // namespace dstools