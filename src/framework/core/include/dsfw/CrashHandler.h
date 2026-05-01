#pragma once

/// @file CrashHandler.h
/// @brief Application crash handler with minidump support.

#include <QString>

#include <functional>

namespace dsfw {

/// @brief Singleton crash handler that captures unhandled exceptions and writes minidump files.
class CrashHandler {
public:
    /// @brief Callback invoked after a crash dump is written.
    using CrashCallback = std::function<void(const QString &dumpPath)>;

    /// @brief Return the singleton instance.
    /// @return Reference to the global CrashHandler.
    static CrashHandler &instance();

    /// @brief Set the directory where minidump files are written.
    /// @param dir Path to the dump output directory.
    void setDumpDirectory(const QString &dir);

    /// @brief Set the callback invoked after a crash dump is created.
    /// @param callback Function called with the path to the dump file.
    void setCrashCallback(CrashCallback callback);

    /// @brief Install the crash handler for the current process.
    void install();

    /// @brief Uninstall the crash handler.
    void uninstall();

    /// @brief Return the current dump output directory.
    /// @return Path to the dump directory.
    QString dumpDirectory() const;

    /// @brief Invoke the crash callback if set. Called by platform-specific handlers.
    void invokeCrashCallback(const QString &dumpPath);

private:
    CrashHandler() = default;
    ~CrashHandler() = default;
    CrashHandler(const CrashHandler &) = delete;
    CrashHandler &operator=(const CrashHandler &) = delete;

    QString m_dumpDir;       ///< Path to the minidump output directory.
    CrashCallback m_callback; ///< Callback invoked after a dump is written.
    bool m_installed = false; ///< Whether the crash handler is currently installed.
};

} // namespace dsfw
