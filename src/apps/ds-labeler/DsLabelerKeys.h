#pragma once
#include <dsfw/AppSettings.h>
#include <dsfw/CommonKeys.h>

namespace DsLabelerKeys {
    using dsfw::CommonKeys::LastDir;
    using dsfw::CommonKeys::ShortcutOpen;
    using dsfw::CommonKeys::NavigationPrev;
    using dsfw::CommonKeys::NavigationNext;

    inline const dstools::SettingsKey<QString> ShortcutSave("Shortcuts/save", "Ctrl+S");
    inline const dstools::SettingsKey<QString> ShortcutUndo("Shortcuts/undo", "Ctrl+Z");
    inline const dstools::SettingsKey<QString> ShortcutRedo("Shortcuts/redo", "Ctrl+Y");
    inline const dstools::SettingsKey<QString> ShortcutExport("Shortcuts/export", "Ctrl+E");
    inline const dstools::SettingsKey<QString> PlaybackPlay("Shortcuts/play", "F5");
    inline const dstools::SettingsKey<QString> PlaybackPlayPause("Shortcuts/playPause", "Space");
    inline const dstools::SettingsKey<QString> PlaybackStop("Shortcuts/stop", "Escape");
    inline const dstools::SettingsKey<QString> ShortcutZoomIn("Shortcuts/zoomIn", "Ctrl+=");
    inline const dstools::SettingsKey<QString> ShortcutZoomOut("Shortcuts/zoomOut", "Ctrl+-");
    inline const dstools::SettingsKey<QString> ShortcutZoomReset("Shortcuts/zoomReset", "Ctrl+0");
    inline const dstools::SettingsKey<QString> ShortcutABCompare("Shortcuts/abCompare", "Ctrl+B");
    inline const dstools::SettingsKey<QString> ShortcutRunFA("Shortcuts/runFA", "Ctrl+R");
    inline const dstools::SettingsKey<QString> ShortcutExtractPitch("Shortcuts/extractPitch", "Ctrl+R");
    inline const dstools::SettingsKey<QString> ShortcutExtractMidi("Shortcuts/extractMidi", "Ctrl+Shift+R");
    inline const dstools::SettingsKey<QString> ShortcutExit("Shortcuts/exit", "Alt+F4");
}
