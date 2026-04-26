#pragma once

/// @file CrashSafeGuard.h
/// @brief Crash recovery detection via session marker file.
///
/// Writes a marker file on startup, deletes it on clean shutdown.
/// On next startup, if the marker still exists, the previous session
/// ended abnormally (crash, kill, power loss).

#include <QString>

namespace dsfw {

    /// @brief Detects whether the previous application session crashed.
    ///
    /// Uses a marker file in the log directory. On startup:
    /// 1. Check if marker exists from previous session → wasPreviousCrash() returns true
    /// 2. Write new marker
    ///
    /// On clean shutdown, the application must call finalize() to remove the marker.
    ///
    /// @code
    /// CrashSafeGuard::markStartup(AppPaths::logDir());
    /// if (CrashSafeGuard::wasPreviousCrash()) {
    ///     // Show user-friendly notification
    /// }
    /// // ... application runs ...
    /// // On clean exit:
    /// CrashSafeGuard::markCleanExit();
    /// @endcode
    class CrashSafeGuard {
    public:
        /// @brief Record session startup. Checks if previous session crashed.
        /// @param markerDir Directory to store the marker file.
        /// Must be called once at application startup.
        static void markStartup(const QString &markerDir);

        /// @brief Remove the marker file on clean shutdown.
        /// Must be called once at application shutdown (e.g. aboutToQuit).
        static void markCleanExit();

        /// @brief Whether the previous session ended abnormally.
        /// @return true if marker existed at startup (previous crash).
        static bool wasPreviousCrash();

    private:
        CrashSafeGuard() = delete;

        static QString m_markerPath;
        static bool m_previousCrash;
    };

} // namespace dsfw