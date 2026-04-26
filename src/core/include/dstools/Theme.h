#pragma once
#include <QApplication>
#include <QColor>

namespace dstools {

class Theme {
public:
    enum Mode { Light, Dark, System };

    /// Apply theme to the entire application
    static void apply(QApplication &app, Mode mode);

    /// Get current theme mode
    static Mode currentMode();

    /// Get theme palette (for custom-painted components like F0Widget)
    struct Palette {
        QColor bgPrimary;       // Main background
        QColor bgSecondary;     // Secondary background (panels/lists)
        QColor bgSurface;       // Cards/overlays
        QColor accent;          // Accent color
        QColor textPrimary;     // Primary text
        QColor textSecondary;   // Secondary text
        QColor border;          // Border
        QColor success;         // Success
        QColor error;           // Error
        QColor warning;         // Warning
    };

    static const Palette &palette();

private:
    static Mode s_mode;
    static Palette s_palette;
};

} // namespace dstools
