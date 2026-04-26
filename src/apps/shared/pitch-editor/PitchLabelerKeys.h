#pragma once
#include <dsfw/AppSettings.h>

namespace PitchLabelerKeys {

    inline const dstools::SettingsKey<bool> ShowPitchTextOverlay("PianoRoll/showPitchTextOverlay", false);
    inline const dstools::SettingsKey<bool> ShowPhonemeTexts("PianoRoll/showPhonemeTexts", true);
    inline const dstools::SettingsKey<bool> ShowCrosshairAndPitch("PianoRoll/showCrosshairAndPitch", true);
    inline const dstools::SettingsKey<bool> SnapToKey("PianoRoll/snapToKey", false);
}
