#pragma once
#include <dsfw/AppSettings.h>

/// @file CommonKeys.h
/// @brief Common settings keys shared across multiple applications.
///
/// Boundary rules for this file:
/// - ONLY keys used by 2+ applications belong here.
/// - Keys specific to a single application go in that app's *Keys.h
///   (e.g., MinLabelKeys, PhonemeLabelerKeys, PitchLabelerKeys,
///    GameInferKeys, PipelineKeys).
/// - Domain-specific constants (DiffSinger format keys, model type
///   identifiers, etc.) must NOT be placed here. They belong in
///   dstools-domain or the relevant app module.
namespace dstools::CommonKeys {
    // General
    inline const SettingsKey<QString> LastDir("General/lastDir", "");
    inline const SettingsKey<int> ThemeMode("General/themeMode", 0);

    // Shortcuts shared by multiple apps
    inline const SettingsKey<QString> ShortcutOpen("Shortcuts/open", "Ctrl+O");
    inline const SettingsKey<QString> NavigationPrev("Shortcuts/prevFile", "PgUp");
    inline const SettingsKey<QString> NavigationNext("Shortcuts/nextFile", "PgDown");
}
