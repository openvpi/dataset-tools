#pragma once

/// @file TranslationManager.h
/// @brief Translation manager for loading Qt .qm translation files.

#include <QString>
#include <QStringList>

namespace dsfw {

/// @brief Manages Qt translation file loading for i18n support.
///
/// Scans search paths for .qm files and installs matching QTranslator instances.
/// Language setting is read from AppSettings on install, or can be specified explicitly.
///
/// Convention:
/// - .qm files named `<target>_<locale>.qm` (e.g. `dsfw-widgets_zh_CN.qm`)
/// - Language setting: "" = follow system, "zh_CN" = Chinese, "en" = English
/// - Language change requires app restart
///
/// @code
/// TranslationManager::install("zh_CN");
/// // UI text now shows Chinese for all loaded .qm files
/// @endcode
class TranslationManager {
public:
    /// @brief Load and install translation files for the given language.
    ///
    /// Scans searchPaths for .qm files matching the locale. If language is empty,
    /// follows the system locale. Installs a QTranslator for each found .qm file.
    ///
    /// @param language Locale string (e.g. "zh_CN", "en"), or empty for system default.
    /// @param searchPaths Directories to scan for .qm files.
    ///        Defaults to ":/translations" (Qt resource) + app dir/translations.
    static void install(const QString &language = {},
                        const QStringList &searchPaths = {});

    /// @brief Return list of available language codes by scanning .qm files.
    /// @param searchPaths Directories to scan. Same defaults as install().
    /// @return List of locale strings (e.g. ["zh_CN", "en"]).
    static QStringList availableLanguages(const QStringList &searchPaths = {});

    /// @brief Return the currently active language.
    /// @return Current locale string, or empty if following system.
    static QString currentLanguage();

private:
    static QString s_currentLanguage;
};

} // namespace dsfw
