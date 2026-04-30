#pragma once
#include <dsfw/AppSettings.h>
#include <dsfw/CommonKeys.h>

/// MinLabel settings key schema -- all persisted keys in one place.
namespace MinLabelKeys {
    // Re-exported from CommonKeys
    using dsfw::CommonKeys::LastDir;
    using dsfw::CommonKeys::ShortcutOpen;
    using dsfw::CommonKeys::NavigationPrev;
    using dsfw::CommonKeys::NavigationNext;

    // Shortcuts
    inline const dstools::SettingsKey<QString> ShortcutExport("Shortcuts/export", "Ctrl+E");
    inline const dstools::SettingsKey<QString> PlaybackPlay("Shortcuts/play", "F5");
}
