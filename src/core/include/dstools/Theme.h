#pragma once
#include <QApplication>
#include <QColor>
#include <QPalette>

namespace dstools {

class Theme {
public:
    enum Mode { Light, Dark };

    struct Palette {
        QColor bgPrimary;
        QColor bgSecondary;
        QColor bgSurface;
        QColor accent;
        QColor textPrimary;
        QColor textSecondary;
        QColor border;
        QColor success;
        QColor error;
        QColor warning;
    };

    /// Apply theme to the entire application
    static void apply(QApplication &app, Mode mode);
    static Mode currentMode();
    static const Palette &palette();

private:
    static Mode s_mode;
    static Palette s_palette;
};

} // namespace dstools
