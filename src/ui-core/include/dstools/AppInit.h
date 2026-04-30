#pragma once
#include <QApplication>

#include <functional>
#include <vector>

namespace dstools {

/// Unified application initialization. Call immediately after QApplication construction in main().
/// Handles: font setup, root privilege check, crash handler.
///
/// Domain-specific initialization (e.g. cpp-pinyin dictionary loading) should be
/// registered via registerPostInitHook() before calling init().
struct AppInit {
    using InitHook = std::function<void(QApplication &app)>;

    /// Register a hook to be called at the end of init(), after core services
    /// are set up. Hooks are called in registration order.
    static void registerPostInitHook(InitHook hook);

    /// @param app              QApplication instance
    /// @param initCrashHandler Whether to initialize QBreakpad (only effective in Release builds)
    /// @return false if the application should exit (e.g. running as root without --allow-root)
    static bool init(QApplication &app, bool initCrashHandler = false);

private:
    static std::vector<InitHook> &hooks();
};

} // namespace dstools
