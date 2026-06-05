#pragma once

/// @file CommonKeys.h
/// @brief Cross-application shared setting keys (directories, theme, shortcuts).

#include <dsfw/AppSettings.h>

/// @brief Common settings keys shared across all dsfw applications.
namespace dsfw::CommonKeys {
    inline const dsfw::SettingsKey<QString> LastDir("General/lastDir", ""); ///< Last-used file dialog directory.
    inline const dsfw::SettingsKey<int> ThemeMode("General/themeMode",
                                                     0); ///< Theme mode (0=Dark, 1=Light, 2=System).

    inline const dsfw::SettingsKey<QString> ShortcutOpen("Shortcuts/open", "Ctrl+O"); ///< Open file shortcut.
    inline const dsfw::SettingsKey<QString> NavigationPrev("Shortcuts/prevFile",
                                                              "PgUp"); ///< Navigate to previous file shortcut.
    inline const dsfw::SettingsKey<QString> NavigationNext("Shortcuts/nextFile",
                                                              "PgDown"); ///< Navigate to next file shortcut.

    inline const dsfw::SettingsKey<QString> WindowGeometry("General/windowGeometry",
                                                              ""); ///< Base64-encoded window geometry.
    inline const dsfw::SettingsKey<QString> WindowState("General/windowState", ""); ///< Base64-encoded window state.
    inline const dsfw::SettingsKey<QString> Language("General/language",
                                                        ""); ///< UI language ("" = system, "zh_CN", "en").

    inline const dsfw::SettingsKey<QString> AudioSplitter("General/splitter/audio",
                                                             ""); ///< AudioVisualizerContainer splitter state.
    inline const dsfw::SettingsKey<QString> PitchSplitter("General/splitter/pitch",
                                                             ""); ///< PitchLabelerPage splitter state.
    inline const dsfw::SettingsKey<QString> MinSplitter("General/splitter/minlabel",
                                                           ""); ///< MinLabelPage splitter state.
    inline const dsfw::SettingsKey<QString> HomeSplitter("General/splitter/home", ""); ///< HomePage splitter state.

    inline const dsfw::SettingsKey<int> SettingsVersion("General/settingsVersion", 1);

    inline const dsfw::SettingsKey<QString> RecentFiles("General/recentFiles", ""); ///< Recent files list.
    inline const dsfw::SettingsKey<QString> WorkspaceLayout("Workspace/layout", "{}"); ///< Workspace layout JSON.
    inline const dsfw::SettingsKey<int> WorkspaceIconSize("Workspace/iconSize", 24); ///< Sidebar icon size.
}
