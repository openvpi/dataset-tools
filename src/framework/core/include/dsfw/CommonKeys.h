#pragma once

/// @file CommonKeys.h
/// @brief Cross-application shared setting keys (directories, theme, shortcuts).

#include <dsfw/AppSettings.h>

/// @brief Common settings keys shared across all dsfw applications.
namespace dsfw::CommonKeys {
    inline const dstools::SettingsKey<QString> LastDir("General/lastDir", "");           ///< Last-used file dialog directory.
    inline const dstools::SettingsKey<int> ThemeMode("General/themeMode", 0);            ///< Theme mode (0=Dark, 1=Light, 2=System).

    inline const dstools::SettingsKey<QString> ShortcutOpen("Shortcuts/open", "Ctrl+O");        ///< Open file shortcut.
    inline const dstools::SettingsKey<QString> NavigationPrev("Shortcuts/prevFile", "PgUp");     ///< Navigate to previous file shortcut.
    inline const dstools::SettingsKey<QString> NavigationNext("Shortcuts/nextFile", "PgDown");   ///< Navigate to next file shortcut.
}
