#pragma once
#include <dstools/AppSettings.h>

/// Common settings keys shared across multiple applications.
/// App-specific keys remain in their respective *Keys.h headers.
namespace dstools::CommonKeys {
    // General
    inline const SettingsKey<QString> LastDir("General/lastDir", "");

    // Shortcuts shared by multiple apps
    inline const SettingsKey<QString> ShortcutOpen("Shortcuts/open", "Ctrl+O");
    inline const SettingsKey<QString> NavigationPrev("Shortcuts/prevFile", "PgUp");
    inline const SettingsKey<QString> NavigationNext("Shortcuts/nextFile", "PgDown");
}
