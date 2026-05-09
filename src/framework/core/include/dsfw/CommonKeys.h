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

    inline const dstools::SettingsKey<QString> WindowGeometry("General/windowGeometry", "");    ///< Base64-encoded window geometry.
    inline const dstools::SettingsKey<QString> WindowState("General/windowState", "");          ///< Base64-encoded window state.
    inline const dstools::SettingsKey<QString> Language("General/language", "");                ///< UI language ("" = system, "zh_CN", "en").

    inline const dstools::SettingsKey<QString> AudioSplitter("General/splitter/audio", "");     ///< AudioVisualizerContainer splitter state.
    inline const dstools::SettingsKey<QString> PitchSplitter("General/splitter/pitch", "");     ///< PitchLabelerPage splitter state.
    inline const dstools::SettingsKey<QString> MinSplitter("General/splitter/minlabel", "");    ///< MinLabelPage splitter state.
    inline const dstools::SettingsKey<QString> HomeSplitter("General/splitter/home", "");       ///< HomePage splitter state.

    inline const dstools::SettingsKey<int> SettingsVersion("General/settingsVersion", 1);
}
