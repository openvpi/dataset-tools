#pragma once

#include <dsfw/AppSettings.h>

namespace dstools::settings {

inline const dstools::SettingsKey<QString> kShortcutSave{"Shortcuts/save", "Ctrl+S"};
inline const dstools::SettingsKey<QString> kShortcutUndo{"Shortcuts/undo", "Ctrl+Z"};
inline const dstools::SettingsKey<QString> kShortcutRedo{"Shortcuts/redo", "Ctrl+Y"};
inline const dstools::SettingsKey<QString> kShortcutZoomIn{"Shortcuts/zoomIn", "Ctrl+="};
inline const dstools::SettingsKey<QString> kShortcutZoomOut{"Shortcuts/zoomOut", "Ctrl+-"};
inline const dstools::SettingsKey<QString> kShortcutZoomReset{"Shortcuts/zoomReset", "Ctrl+0"};
inline const dstools::SettingsKey<QString> kShortcutABCompare{"Shortcuts/abCompare", "Ctrl+B"};
inline const dstools::SettingsKey<QString> kShortcutPlayPause{"Shortcuts/playPause", "Space"};
inline const dstools::SettingsKey<QString> kShortcutFA{"Shortcuts/fa", "F"};
inline const dstools::SettingsKey<QString> kShortcutExtractPitch{"Shortcuts/extractPitch", "F"};
inline const dstools::SettingsKey<QString> kShortcutExtractMidi{"Shortcuts/extractMidi", "M"};
inline const dstools::SettingsKey<QString> kShortcutASR{"Shortcuts/asr", "R"};
inline const dstools::SettingsKey<QString> kShortcutLyricFA{"Shortcuts/lyricfa", "L"};

inline const dstools::SettingsKey<QString> kEditorSplitterState{"Layout/editorSplitterState", ""};
inline const dstools::SettingsKey<QString> kSplitterState{"Layout/splitterState", ""};

inline const dstools::SettingsKey<bool> kAutoSaveEnabled{"General/autoSaveEnabled", true};
inline const dstools::SettingsKey<int> kAutoSaveIntervalMs{"General/autoSaveIntervalMs", 30000};

inline const dstools::SettingsKey<QString> kChartVisible{"ViewLayout/chartVisible", ""};
inline const dstools::SettingsKey<QString> kChartOrder{"ViewLayout/chartOrder", ""};
inline const dstools::SettingsKey<int> kDefaultResolution{"AudioVisualizer/defaultResolution", 0};

inline const dstools::SettingsKey<QString> kLastSlice{"State/lastSlice", ""};

} // namespace dstools::settings