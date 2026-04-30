#pragma once
#include <dsfw/AppSettings.h>

namespace dsfw::CommonKeys {
    inline const dstools::SettingsKey<QString> LastDir("General/lastDir", "");
    inline const dstools::SettingsKey<int> ThemeMode("General/themeMode", 0);

    inline const dstools::SettingsKey<QString> ShortcutOpen("Shortcuts/open", "Ctrl+O");
    inline const dstools::SettingsKey<QString> NavigationPrev("Shortcuts/prevFile", "PgUp");
    inline const dstools::SettingsKey<QString> NavigationNext("Shortcuts/nextFile", "PgDown");
}
