#pragma once
#include <dstools/AppSettings.h>
#include <dstools/CommonKeys.h>

/// SlurCutter settings key schema -- all persisted keys in one place.
namespace SlurCutterKeys {
    // Re-exported from CommonKeys
    using dstools::CommonKeys::LastDir;
    using dstools::CommonKeys::ShortcutOpen;
    using dstools::CommonKeys::NavigationPrev;
    using dstools::CommonKeys::NavigationNext;

    // Shortcuts
    inline const dstools::SettingsKey<QString> PlaybackPlay("Shortcuts/play", "F5");

    // F0Widget display options
    inline const dstools::SettingsKey<bool> SnapToKey("F0Widget/snapToKey", false);
    inline const dstools::SettingsKey<bool> ShowPitchTextOverlay("F0Widget/showPitchTextOverlay", false);
    inline const dstools::SettingsKey<bool> ShowPhonemeTexts("F0Widget/showPhonemeTexts", false);
    inline const dstools::SettingsKey<bool> ShowCrosshairAndPitch("F0Widget/showCrosshairAndPitch", false);
}
