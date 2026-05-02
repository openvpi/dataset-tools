#pragma once
#include <dsfw/AppSettings.h>
#include <dsfw/CommonKeys.h>

/// PhonemeLabeler settings key schema -- all persisted keys in one place.
namespace PhonemeLabelerKeys {
    // Re-exported from CommonKeys
    using dsfw::CommonKeys::LastDir;
    using dsfw::CommonKeys::ShortcutOpen;
    using dsfw::CommonKeys::NavigationPrev;
    using dsfw::CommonKeys::NavigationNext;

    // Shortcuts
    inline const dstools::SettingsKey<QString> PlaybackPlayPause("Shortcuts/playPause", "Space");
    inline const dstools::SettingsKey<QString> PlaybackStop("Shortcuts/stop", "Escape");
    inline const dstools::SettingsKey<QString> ShortcutSave("Shortcuts/save", "Ctrl+S");
    inline const dstools::SettingsKey<QString> ShortcutSaveAs("Shortcuts/saveAs", "Ctrl+Shift+S");
    inline const dstools::SettingsKey<QString> ShortcutUndo("Shortcuts/undo", "Ctrl+Z");
    inline const dstools::SettingsKey<QString> ShortcutRedo("Shortcuts/redo", "Ctrl+Y");
    inline const dstools::SettingsKey<QString> ShortcutZoomIn("Shortcuts/zoomIn", "Ctrl+=");
    inline const dstools::SettingsKey<QString> ShortcutZoomOut("Shortcuts/zoomOut", "Ctrl+-");
    inline const dstools::SettingsKey<QString> ShortcutZoomReset("Shortcuts/zoomReset", "Ctrl+0");
    inline const dstools::SettingsKey<QString> ShortcutToggleBinding("Shortcuts/toggleBinding", "B");
    inline const dstools::SettingsKey<QString> ShortcutExit("Shortcuts/exit", "Alt+F4");

    // View settings
    inline const dstools::SettingsKey<double> ZoomLevel("View/zoomLevel", 200.0);
    inline const dstools::SettingsKey<bool> BoundaryBindingEnabled("View/boundaryBindingEnabled", true);
    inline const dstools::SettingsKey<double> BoundaryBindingTolerance("View/boundaryBindingTolerance", 1.0);
    inline const dstools::SettingsKey<bool> PowerEnabled("View/powerEnabled", true);
    inline const dstools::SettingsKey<bool> SpectrogramEnabled("View/spectrogramEnabled", true);
    inline const dstools::SettingsKey<QString> SpectrogramColorStyle("View/spectrogramColorStyle", "Orange-Yellow");

    // Window state (stored as base64-encoded QByteArray)
    inline const dstools::SettingsKey<QString> WindowGeometry("Window/geometry", "");
    inline const dstools::SettingsKey<QString> WindowState("Window/state", "");
}
