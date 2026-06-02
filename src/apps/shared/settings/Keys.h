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
inline const dstools::SettingsKey<int> kBackupKeepCount{"General/backupKeepCount", 10};

inline const dstools::SettingsKey<QString> kChartVisible{"ViewLayout/chartVisible", ""};
inline const dstools::SettingsKey<QString> kChartOrder{"ViewLayout/chartOrder", ""};
inline const dstools::SettingsKey<int> kDefaultResolution{"AudioVisualizer/defaultResolution", 0};

inline const dstools::SettingsKey<QString> kLastSlice{"State/lastSlice", ""};

} // namespace dstools::settings

namespace dstools::settings::pitch {

inline const dstools::SettingsKey<bool> kShowPitchTextOverlay("PianoRoll/showPitchTextOverlay", false);
inline const dstools::SettingsKey<bool> kShowPhonemeTexts("PianoRoll/showPhonemeTexts", true);
inline const dstools::SettingsKey<bool> kShowCrosshairAndPitch("PianoRoll/showCrosshairAndPitch", true);
inline const dstools::SettingsKey<bool> kSnapToKey("PianoRoll/snapToKey", false);
inline const dstools::SettingsKey<double> kVScale("PianoRoll/vScale", 20.0);
inline const dstools::SettingsKey<double> kBoundaryHitRadius("PianoRoll/boundaryHitRadius", 5.0);
inline const dstools::SettingsKey<int> kDefaultResolution("PianoRoll/defaultResolution", 40);
inline const dstools::SettingsKey<double> kModulationDragSensitivity("PianoRoll/modulationDragSensitivity", 80.0);

} // namespace dstools::settings::pitch

namespace dstools::settings::app {

inline const dstools::SettingsKey<QString> kLanguage{"App/language", ""};
inline const dstools::SettingsKey<QString> kLastProjectDir{"App/lastProjectDir", ""};
inline const dstools::SettingsKey<QString> kRecentProjects{"App/recentProjects", ""};
inline const dstools::SettingsKey<QString> kAudioDevice{"App/audioDevice", ""};

} // namespace dstools::settings::app

namespace dstools::settings::inference {

inline const dstools::SettingsKey<QString> kGlobalProvider{"Settings/globalProvider", "cpu"};
inline const dstools::SettingsKey<int> kDeviceIndex{"Settings/deviceIndex", 0};
inline const dstools::SettingsKey<QString> kG2pEngine{"Settings/g2pEngine", "pinyin"};
inline const dstools::SettingsKey<QString> kDictPath{"Settings/dictPath", ""};
inline const dstools::SettingsKey<QString> kFaNonSpeechPh{"Settings/faNonSpeechPh", "AP, SP"};
inline const dstools::SettingsKey<QString> kPitchUvVocab{"Settings/pitchUvVocab", "AP, SP, br, sil"};
inline const dstools::SettingsKey<int> kPitchUvWordCond{"Settings/pitchUvWordCond", 1};
inline const dstools::SettingsKey<double> kPitchMinF0{"Settings/pitchMinF0", 50.0};
inline const dstools::SettingsKey<double> kPitchMaxF0{"Settings/pitchMaxF0", 1100.0};

inline constexpr const char *kModelsGroupFmt = "Settings/Models/%1";
inline constexpr const char *kModelsModelPath = "modelPath";
inline constexpr const char *kModelsProvider = "provider";
inline constexpr const char *kModelsForceCpu = "forceCpu";

inline constexpr const char *kPreloadGroupFmt = "Settings/Preload/%1";
inline constexpr const char *kPreloadEnabled = "enabled";
inline constexpr const char *kPreloadCount = "count";

inline constexpr const char *kPhNumLanguagesGroup = "Settings/PhNumLanguages";
inline constexpr const char *kPhNumLanguagesDictPath = "dictPath";
inline constexpr const char *kPhNumLanguagesVowelsPath = "vowelsPath";
inline constexpr const char *kPhNumLanguagesConsonantsPath = "consonantsPath";

} // namespace dstools::settings::inference