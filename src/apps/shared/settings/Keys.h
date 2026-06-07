#pragma once

#include <dsfw/AppSettings.h>

namespace dstools::settings {

inline const dsfw::SettingsKey<QString> kShortcutSave{"Shortcuts/save", "Ctrl+S"};
inline const dsfw::SettingsKey<QString> kShortcutUndo{"Shortcuts/undo", "Ctrl+Z"};
inline const dsfw::SettingsKey<QString> kShortcutRedo{"Shortcuts/redo", "Ctrl+Y"};
inline const dsfw::SettingsKey<QString> kShortcutZoomIn{"Shortcuts/zoomIn", "Ctrl+="};
inline const dsfw::SettingsKey<QString> kShortcutZoomOut{"Shortcuts/zoomOut", "Ctrl+-"};
inline const dsfw::SettingsKey<QString> kShortcutZoomReset{"Shortcuts/zoomReset", "Ctrl+0"};
inline const dsfw::SettingsKey<QString> kShortcutABCompare{"Shortcuts/abCompare", "Ctrl+B"};
inline const dsfw::SettingsKey<QString> kShortcutPlayPause{"Shortcuts/playPause", "Space"};
inline const dsfw::SettingsKey<QString> kShortcutFA{"Shortcuts/fa", "F"};
inline const dsfw::SettingsKey<QString> kShortcutExtractPitch{"Shortcuts/extractPitch", "F"};
inline const dsfw::SettingsKey<QString> kShortcutExtractMidi{"Shortcuts/extractMidi", "M"};
inline const dsfw::SettingsKey<QString> kShortcutASR{"Shortcuts/asr", "R"};
inline const dsfw::SettingsKey<QString> kShortcutLyricFA{"Shortcuts/lyricfa", "L"};

inline const dsfw::SettingsKey<QString> kEditorSplitterState{"Layout/editorSplitterState", ""};
inline const dsfw::SettingsKey<QString> kSplitterState{"Layout/splitterState", ""};

inline const dsfw::SettingsKey<bool> kAutoSaveEnabled{"General/autoSaveEnabled", true};
inline const dsfw::SettingsKey<int> kAutoSaveIntervalMs{"General/autoSaveIntervalMs", 30000};
inline const dsfw::SettingsKey<int> kBackupKeepCount{"General/backupKeepCount", 10};

inline const dsfw::SettingsKey<QString> kChartVisible{"ViewLayout/chartVisible", ""};
inline const dsfw::SettingsKey<QString> kChartOrder{"ViewLayout/chartOrder", ""};
inline const dsfw::SettingsKey<int> kDefaultResolution{"AudioVisualizer/defaultResolution", 0};

inline const dsfw::SettingsKey<QString> kLastSlice{"State/lastSlice", ""};

inline const dsfw::SettingsKey<QString> kHSplitterState{"Layout/hSplitterState", ""};
inline const dsfw::SettingsKey<QString> kContentSplitterState{"Layout/contentSplitterState", ""};
inline const dsfw::SettingsKey<QString> kContainerSplitterState{"Layout/containerSplitterState", ""};
inline const dsfw::SettingsKey<QString> kLayoutChartOrder{"Layout/chartOrder", ""};
inline const dsfw::SettingsKey<bool> kSidebarCollapsed{"Layout/sidebarCollapsed", false};

inline const dsfw::SettingsKey<QString> kLyricDir{"LyricFA/libraryPath", ""};

} // namespace dstools::settings

namespace dstools::settings::pitch {

inline const dsfw::SettingsKey<bool> kShowPitchTextOverlay("PianoRoll/showPitchTextOverlay", false);
inline const dsfw::SettingsKey<bool> kShowPhonemeTexts("PianoRoll/showPhonemeTexts", true);
inline const dsfw::SettingsKey<bool> kShowCrosshairAndPitch("PianoRoll/showCrosshairAndPitch", true);
inline const dsfw::SettingsKey<bool> kSnapToKey("PianoRoll/snapToKey", false);
inline const dsfw::SettingsKey<double> kVScale("PianoRoll/vScale", 20.0);
inline const dsfw::SettingsKey<double> kBoundaryHitRadius("PianoRoll/boundaryHitRadius", 5.0);
inline const dsfw::SettingsKey<int> kDefaultResolution("PianoRoll/defaultResolution", 40);
inline const dsfw::SettingsKey<double> kModulationDragSensitivity("PianoRoll/modulationDragSensitivity", 80.0);

} // namespace dstools::settings::pitch

namespace dstools::settings::app {

inline const dsfw::SettingsKey<QString> kLanguage{"App/language", ""};
inline const dsfw::SettingsKey<QString> kLastProjectDir{"App/lastProjectDir", ""};
inline const dsfw::SettingsKey<QString> kRecentProjects{"App/recentProjects", ""};
inline const dsfw::SettingsKey<QString> kAudioDevice{"App/audioDevice", ""};

} // namespace dstools::settings::app

namespace dstools::settings::inference {

inline const dsfw::SettingsKey<QString> kGlobalProvider{"Settings/globalProvider", "cpu"};
inline const dsfw::SettingsKey<int> kDeviceIndex{"Settings/deviceIndex", 0};
inline const dsfw::SettingsKey<QString> kG2pEngine{"Settings/g2pEngine", "pinyin"};
inline const dsfw::SettingsKey<QString> kDictPath{"Settings/dictPath", ""};
inline const dsfw::SettingsKey<QString> kFaNonSpeechPh{"Settings/faNonSpeechPh", "AP, SP"};
inline const dsfw::SettingsKey<QString> kPitchUvVocab{"Settings/pitchUvVocab", "AP, SP, br, sil"};
inline const dsfw::SettingsKey<int> kPitchUvWordCond{"Settings/pitchUvWordCond", 1};
inline const dsfw::SettingsKey<double> kPitchMinF0{"Settings/pitchMinF0", 50.0};
inline const dsfw::SettingsKey<double> kPitchMaxF0{"Settings/pitchMaxF0", 1100.0};

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