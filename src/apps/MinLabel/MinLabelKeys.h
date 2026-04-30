#pragma once
#include <dsfw/AppSettings.h>
#include <dstools/CommonKeys.h>

/// MinLabel settings key schema -- all persisted keys in one place.
namespace MinLabelKeys {
    // Re-exported from CommonKeys
    using dstools::CommonKeys::LastDir;
    using dstools::CommonKeys::ShortcutOpen;
    using dstools::CommonKeys::NavigationPrev;
    using dstools::CommonKeys::NavigationNext;

    // Shortcuts
    inline const dstools::SettingsKey<QString> ShortcutExport("Shortcuts/export", "Ctrl+E");
    inline const dstools::SettingsKey<QString> PlaybackPlay("Shortcuts/play", "F5");
}
