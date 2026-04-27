#pragma once
#include <dstools/AppSettings.h>

/// MinLabel settings key schema -- all persisted keys in one place.
namespace MinLabelKeys {
    // General
    inline const dstools::SettingsKey<QString> LastDir("General/lastDir", "");

    // Shortcuts
    inline const dstools::SettingsKey<QString> ShortcutOpen("Shortcuts/open", "Ctrl+O");
    inline const dstools::SettingsKey<QString> ShortcutExport("Shortcuts/export", "Ctrl+E");
    inline const dstools::SettingsKey<QString> NavigationPrev("Shortcuts/prevFile", "PgUp");
    inline const dstools::SettingsKey<QString> NavigationNext("Shortcuts/nextFile", "PgDown");
    inline const dstools::SettingsKey<QString> PlaybackPlay("Shortcuts/play", "F5");
}
