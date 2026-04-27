#pragma once

/// @file Theme.h
/// @brief Application theme manager with Dark/Light/FollowSystem support.
///
/// Usage:
/// @code
///   // In main.cpp — initialize once:
///   dstools::Theme::instance().init(app);  // reads saved pref, defaults to Dark
///
///   // In any MainWindow — add theme menu:
///   auto *viewMenu = menuBar()->addMenu(tr("View(&V)"));
///   dstools::Theme::instance().populateThemeMenu(viewMenu);
///
///   // React to theme changes:
///   connect(&dstools::Theme::instance(), &dstools::Theme::themeChanged, this, [this]() {
///       // update custom-painted widgets
///   });
/// @endcode

#include <QApplication>
#include <QColor>
#include <QObject>
#include <QPalette>

class QActionGroup;
class QMenu;

namespace dstools {

class Theme : public QObject {
    Q_OBJECT

public:
    enum Mode { Dark, Light, FollowSystem };
    Q_ENUM(Mode)

    struct Palette {
        QColor bgPrimary;
        QColor bgSecondary;
        QColor bgSurface;
        QColor bgHover;
        QColor accent;
        QColor textPrimary;
        QColor textSecondary;
        QColor textDisabled;
        QColor border;
        QColor borderLight;
        QColor success;
        QColor error;
        QColor warning;
    };

    /// Global singleton.
    static Theme &instance();

    /// Initialize theme system. Call once from main(), before showing any window.
    /// Reads saved preference from AppSettings key "theme" (per-app).
    /// @param defaultMode  fallback if no saved preference exists
    void init(QApplication &app, Mode defaultMode = Dark);

    /// Switch theme at runtime (persists the choice).
    void setMode(Mode mode);

    /// Current mode setting (may be FollowSystem).
    Mode mode() const;

    /// Whether the currently active appearance is dark.
    bool isDark() const;

    /// Semantic color palette for the active theme.
    const Palette &palette() const;

    /// Add "Dark / Light / Follow System" radio actions to a menu.
    void populateThemeMenu(QMenu *menu);

signals:
    /// Emitted after theme switch completes. Connect to repaint custom widgets.
    void themeChanged();

private:
    explicit Theme(QObject *parent = nullptr);
    ~Theme() override = default;

    void applyTheme(bool dark);
    bool resolveIsDark() const;
    void onSystemSchemeChanged();

    QApplication *m_app = nullptr;
    Mode m_mode = Dark;
    bool m_isDark = true;
    Palette m_palette;
    QActionGroup *m_actionGroup = nullptr;
};

} // namespace dstools
