#pragma once
#include <dsfw/AppSettings.h>
#include <dsfw/CommonKeys.h>

namespace PitchLabelerKeys {
    using dsfw::CommonKeys::LastDir;
    using dsfw::CommonKeys::ShortcutOpen;
    using dsfw::CommonKeys::NavigationPrev;
    using dsfw::CommonKeys::NavigationNext;

    inline const dstools::SettingsKey<QString> PlaybackPlayPause("Shortcuts/playPause", "Space");
    inline const dstools::SettingsKey<QString> PlaybackStop("Shortcuts/stop", "Escape");
    inline const dstools::SettingsKey<QString> SeekForward("Shortcuts/seekForward", "Right");
    inline const dstools::SettingsKey<QString> SeekBackward("Shortcuts/seekBackward", "Left");
    inline const dstools::SettingsKey<QString> ToolSelect("Shortcuts/toolSelect", "V");
    inline const dstools::SettingsKey<QString> ToolModulation("Shortcuts/toolModulation", "M");
    inline const dstools::SettingsKey<QString> ToolDrift("Shortcuts/toolDrift", "D");
    inline const dstools::SettingsKey<QString> ShortcutSave("Shortcuts/save", "Ctrl+S");
    inline const dstools::SettingsKey<QString> ShortcutSaveAll("Shortcuts/saveAll", "Ctrl+Shift+S");
    inline const dstools::SettingsKey<QString> ShortcutUndo("Shortcuts/undo", "Ctrl+Z");
    inline const dstools::SettingsKey<QString> ShortcutRedo("Shortcuts/redo", "Ctrl+Y");
    inline const dstools::SettingsKey<QString> ShortcutZoomIn("Shortcuts/zoomIn", "Ctrl+=");
    inline const dstools::SettingsKey<QString> ShortcutZoomOut("Shortcuts/zoomOut", "Ctrl+-");
    inline const dstools::SettingsKey<QString> ShortcutZoomReset("Shortcuts/zoomReset", "Ctrl+0");
    inline const dstools::SettingsKey<QString> ShortcutABCompare("Shortcuts/abCompare", "Ctrl+B");
    inline const dstools::SettingsKey<QString> ShortcutExit("Shortcuts/exit", "Alt+F4");

    inline const dstools::SettingsKey<bool> ShowPitchTextOverlay("PianoRoll/showPitchTextOverlay", false);
    inline const dstools::SettingsKey<bool> ShowPhonemeTexts("PianoRoll/showPhonemeTexts", true);
    inline const dstools::SettingsKey<bool> ShowCrosshairAndPitch("PianoRoll/showCrosshairAndPitch", true);
    inline const dstools::SettingsKey<bool> SnapToKey("PianoRoll/snapToKey", false);
}
