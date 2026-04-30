#pragma once

/// @file FramelessHelper.h
/// @brief Helper for applying frameless window styling via QWindowKit.

#include <QDialog>
#include <QMainWindow>

namespace dsfw {

/// @brief Utility for making windows frameless with custom title bar support.
///
/// Uses QWindowKit under the hood to remove the native frame while preserving
/// platform-specific window management (resize, snap, shadow).
struct FramelessHelper {
    /// @brief Apply frameless styling to a QMainWindow.
    /// @param window Target main window.
    static void apply(QMainWindow *window);

    /// @brief Apply frameless styling to a QDialog.
    /// @param dialog Target dialog.
    static void applyToDialog(QDialog *dialog);
};

} // namespace dsfw
