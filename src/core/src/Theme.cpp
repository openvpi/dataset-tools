#include <dstools/Theme.h>
#include <QFile>
#include <QTextStream>

namespace dstools {

Theme::Mode Theme::s_mode = Theme::Dark;
Theme::Palette Theme::s_palette = {};

static Theme::Palette makeDarkPalette() {
    Theme::Palette p;
    p.bgPrimary     = QColor(0x1E, 0x1E, 0x2E);  // #1E1E2E
    p.bgSecondary   = QColor(0x28, 0x28, 0x3C);  // #28283C
    p.bgSurface     = QColor(0x31, 0x31, 0x4A);  // #31314A
    p.accent        = QColor(0x7A, 0xA2, 0xF7);  // #7AA2F7 (blue)
    p.textPrimary   = QColor(0xC6, 0xD0, 0xF5);  // #C6D0F5
    p.textSecondary = QColor(0x8B, 0x92, 0xA8);  // #8B92A8
    p.border        = QColor(0x44, 0x44, 0x5A);  // #44445A
    p.success       = QColor(0xA6, 0xE3, 0xA1);  // #A6E3A1
    p.error         = QColor(0xF3, 0x8B, 0xA8);  // #F38BA8
    p.warning       = QColor(0xFA, 0xB3, 0x87);  // #FAB387
    return p;
}

static Theme::Palette makeLightPalette() {
    Theme::Palette p;
    p.bgPrimary     = QColor(0xEF, 0xF1, 0xF5);
    p.bgSecondary   = QColor(0xE6, 0xE9, 0xEF);
    p.bgSurface     = QColor(0xFF, 0xFF, 0xFF);
    p.accent        = QColor(0x1E, 0x66, 0xF5);
    p.textPrimary   = QColor(0x4C, 0x4F, 0x69);
    p.textSecondary = QColor(0x6C, 0x6F, 0x85);
    p.border        = QColor(0xCC, 0xD0, 0xDA);
    p.success       = QColor(0x40, 0xA0, 0x2B);
    p.error         = QColor(0xD2, 0x00, 0x3F);
    p.warning       = QColor(0xFE, 0x64, 0x0B);
    return p;
}

void Theme::apply(QApplication &app, Mode mode) {
    s_mode = mode;

    if (mode == Dark) {
        s_palette = makeDarkPalette();
    } else {
        s_palette = makeLightPalette();
    }

    // Load QSS from resources if available
    QString qssPath = (mode == Dark) ? ":/themes/dark.qss" : ":/themes/light.qss";
    QFile qssFile(qssPath);
    if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(qssFile.readAll());
        qssFile.close();
    }

    // Set application palette for non-QSS styled widgets
    QPalette pal;
    pal.setColor(QPalette::Window, s_palette.bgPrimary);
    pal.setColor(QPalette::WindowText, s_palette.textPrimary);
    pal.setColor(QPalette::Base, s_palette.bgSecondary);
    pal.setColor(QPalette::AlternateBase, s_palette.bgSurface);
    pal.setColor(QPalette::Text, s_palette.textPrimary);
    pal.setColor(QPalette::Button, s_palette.bgSurface);
    pal.setColor(QPalette::ButtonText, s_palette.textPrimary);
    pal.setColor(QPalette::Highlight, s_palette.accent);
    pal.setColor(QPalette::HighlightedText, QColor(0xFF, 0xFF, 0xFF));
    pal.setColor(QPalette::ToolTipBase, s_palette.bgSurface);
    pal.setColor(QPalette::ToolTipText, s_palette.textPrimary);
    pal.setColor(QPalette::PlaceholderText, s_palette.textSecondary);
    pal.setColor(QPalette::Disabled, QPalette::Text, s_palette.textSecondary);
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, s_palette.textSecondary);
    app.setPalette(pal);
}

Theme::Mode Theme::currentMode() {
    return s_mode;
}

const Theme::Palette &Theme::palette() {
    return s_palette;
}

} // namespace dstools
