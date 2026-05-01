#pragma once

#include <QString>

namespace dsfw {

class AppPaths {
public:
    static QString configDir();
    static QString logDir();
    static QString dumpDir();
    static QString cacheDir();
    static QString dataDir();

    static void migrateFromLegacyPaths();

private:
    static QString ensureDir(const QString &path);
};

} // namespace dsfw
