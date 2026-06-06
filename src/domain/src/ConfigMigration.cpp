#include <dstools/ConfigMigration.h>

namespace dstools {

void ConfigMigration::registerMigration(int targetVersion, MigrationFunc func) {
    m_migrations[targetVersion] = std::move(func);
}

Result<void> ConfigMigration::migrate(nlohmann::json &json, int currentConfigVersion) const {
    for (int target = currentConfigVersion + 1;; ++target) {
        auto it = m_migrations.find(target);
        if (it == m_migrations.end())
            break;

        if (!it->second(json)) {
            return Result<void>::Error("Config migration to version " + std::to_string(target) + " failed");
        }
    }
    return Result<void>::Ok();
}

} // namespace dstools