#pragma once
#include <dstools/AppSettings.h>

/// PitchLabeler settings key schema -- all persisted keys in one place.
namespace PitchLabelerKeys {
    // General
    inline const dstools::SettingsKey<QString> LastDir("General/lastDir", "");

    // Shortcuts
    inline const dstools::SettingsKey<QString> ShortcutOpen("Shortcuts/open", "Ctrl+O");
    inline const dstools::SettingsKey<QString> NavigationPrev("Shortcuts/prevFile", "PgUp");
    inline const dstools::SettingsKey<QString> NavigationNext("Shortcuts/nextFile", "PgDown");
    inline const dstools::SettingsKey<QString> PlaybackPlayPause("Shortcuts/playPause", "Space");
    inline const dstools::SettingsKey<QString> PlaybackStop("Shortcuts/stop", "Escape");
    inline const dstools::SettingsKey<QString> SeekForward("Shortcuts/seekForward", "Right");
    inline const dstools::SettingsKey<QString> SeekBackward("Shortcuts/seekBackward", "Left");
    inline const dstools::SettingsKey<QString> ToolSelect("Shortcuts/toolSelect", "V");
    inline const dstools::SettingsKey<QString> ToolModulation("Shortcuts/toolModulation", "M");
    inline const dstools::SettingsKey<QString> ToolDrift("Shortcuts/toolDrift", "D");

    // PianoRoll display options (matching SlurCutter's F0Widget pattern)
    inline const dstools::SettingsKey<bool> ShowPitchTextOverlay("PianoRoll/showPitchTextOverlay", false);
    inline const dstools::SettingsKey<bool> ShowPhonemeTexts("PianoRoll/showPhonemeTexts", true);
    inline const dstools::SettingsKey<bool> ShowCrosshairAndPitch("PianoRoll/showCrosshairAndPitch", true);
    inline const dstools::SettingsKey<bool> SnapToKey("PianoRoll/snapToKey", false);
}
