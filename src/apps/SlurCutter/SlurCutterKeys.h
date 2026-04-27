#pragma once
#include <dstools/AppSettings.h>

/// SlurCutter settings key schema -- all persisted keys in one place.
namespace SlurCutterKeys {
    // General
    inline const dstools::SettingsKey<QString> LastDir("General/lastDir", "");

    // Shortcuts
    inline const dstools::SettingsKey<QString> ShortcutOpen("Shortcuts/open", "Ctrl+O");
    inline const dstools::SettingsKey<QString> NavigationPrev("Shortcuts/prevFile", "PgUp");
    inline const dstools::SettingsKey<QString> NavigationNext("Shortcuts/nextFile", "PgDown");
    inline const dstools::SettingsKey<QString> PlaybackPlay("Shortcuts/play", "F5");

    // F0Widget display options
    inline const dstools::SettingsKey<bool> SnapToKey("F0Widget/snapToKey", false);
    inline const dstools::SettingsKey<bool> ShowPitchTextOverlay("F0Widget/showPitchTextOverlay", false);
    inline const dstools::SettingsKey<bool> ShowPhonemeTexts("F0Widget/showPhonemeTexts", false);
    inline const dstools::SettingsKey<bool> ShowCrosshairAndPitch("F0Widget/showCrosshairAndPitch", false);
}
