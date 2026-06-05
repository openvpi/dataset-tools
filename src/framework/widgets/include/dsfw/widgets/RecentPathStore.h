#pragma once
/// @file RecentPathStore.h
/// @brief Shared recent-path persistence for path selection widgets.

#include <QString>
#include <dsfw/widgets/WidgetsGlobal.h>

namespace dsfw::widgets {

    /// @brief Utility class for persisting recently browsed paths via AppSettings.
    ///
    /// Both PathSelector and FilePathSelector share the same storage group
    /// ("PathSelector") so that a path selected in one widget is remembered
    /// when the other is used with the same settings key.
    class DSFW_WIDGETS_API RecentPathStore {
    public:
        /// @brief Load the most recently used directory for the given key.
        /// @param settingsKey Settings key (empty returns empty string).
        /// @return The stored directory path, or empty if not found.
        static QString load(const QString &settingsKey);

        /// @brief Save a recently used path, extracting the directory if it's a file.
        /// @param settingsKey Settings key (empty = no-op).
        /// @param path The selected file or directory path.
        static void save(const QString &settingsKey, const QString &path);
    };

} // namespace dsfw::widgets
