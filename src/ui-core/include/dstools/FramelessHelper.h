#pragma once

/// @file FramelessHelper.h
/// @brief Utility to apply QWindowKit frameless window decoration to any QMainWindow.
///
/// Usage (in main.cpp, after window construction):
/// @code
///   MainWindow w;
///   dstools::FramelessHelper::apply(&w);
///   w.show();
/// @endcode

#include <QDialog>
#include <QMainWindow>

namespace dstools {

struct FramelessHelper {
    /// Make @p window frameless using QWindowKit, with a custom title bar
    /// containing min/max/close buttons. The title bar respects the current theme.
    /// @param window  The QMainWindow to decorate. Must not be null.
    static void apply(QMainWindow *window);

    /// Make @p dialog frameless using QWindowKit, with a minimal title bar
    /// containing only a close button. The title bar respects the current theme.
    /// @param dialog  The QDialog to decorate. Must not be null.
    static void applyToDialog(QDialog *dialog);
};

} // namespace dstools
