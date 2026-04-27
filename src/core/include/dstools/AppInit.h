#pragma once
#include <QApplication>

namespace dstools {

/// Unified application initialization. Call immediately after QApplication construction in main().
/// Handles: font setup, root privilege check, crash handler, cpp-pinyin dictionary loading.
struct AppInit {
    /// @param app          QApplication instance
    /// @param initPinyin   Whether to load cpp-pinyin dictionary (MinLabel/LyricFA need this)
    /// @param initCrashHandler Whether to initialize QBreakpad (only effective in Release builds)
    /// @return false if the application should exit (e.g. running as root without --allow-root)
    static bool init(QApplication &app,
                     bool initPinyin = false,
                     bool initCrashHandler = false);
};

} // namespace dstools
