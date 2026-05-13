#pragma once

namespace dstools::AppSettingKeys {

inline constexpr const char *GlobalProvider = "Settings/globalProvider";
inline constexpr const char *DeviceIndex = "Settings/deviceIndex";

inline constexpr const char *ModelsGroupFmt = "Settings/Models/%1";
inline constexpr const char *ModelsModelPath = "modelPath";
inline constexpr const char *ModelsProvider = "provider";
inline constexpr const char *ModelsForceCpu = "forceCpu";

inline constexpr const char *PreloadGroupFmt = "Settings/Preload/%1";
inline constexpr const char *PreloadEnabled = "enabled";
inline constexpr const char *PreloadCount = "count";

inline constexpr const char *G2pEngine = "Settings/g2pEngine";
inline constexpr const char *DictPath = "Settings/dictPath";

inline constexpr const char *FaNonSpeechPh = "Settings/faNonSpeechPh";

inline constexpr const char *PitchUvVocab = "Settings/pitchUvVocab";
inline constexpr const char *PitchUvWordCond = "Settings/pitchUvWordCond";

inline constexpr const char *PhNumLanguagesGroup = "Settings/PhNumLanguages";
inline constexpr const char *PhNumLanguagesDictPath = "dictPath";
inline constexpr const char *PhNumLanguagesVowelsPath = "vowelsPath";
inline constexpr const char *PhNumLanguagesConsonantsPath = "consonantsPath";

inline constexpr const char *Language = "App/language";
inline constexpr const char *LastProjectDir = "App/lastProjectDir";
inline constexpr const char *RecentProjects = "App/recentProjects";

} // namespace dstools::AppSettingKeys