#pragma once
#include <dsfw/AppSettings.h>

namespace dsfw::CommonKeys {
    inline const SettingsKey<QString> LastDir("General/lastDir", "");
    inline const SettingsKey<int> ThemeMode("General/themeMode", 0);

    inline const SettingsKey<QString> ShortcutOpen("Shortcuts/open", "Ctrl+O");
    inline const SettingsKey<QString> NavigationPrev("Shortcuts/prevFile", "PgUp");
    inline const SettingsKey<QString> NavigationNext("Shortcuts/nextFile", "PgDown");
}
