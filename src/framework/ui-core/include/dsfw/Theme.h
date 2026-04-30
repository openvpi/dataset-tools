#pragma once

/// @file Theme.h
/// @brief Application theme manager with Dark/Light/System modes and domain-specific palettes.

#include <dsfw/AppSettings.h>

#include <QApplication>
#include <QColor>
#include <QObject>
#include <QPalette>
#include <memory>

class QActionGroup;
class QMenu;

namespace dsfw {

    /// @brief Application-wide theme manager singleton.
    ///
    /// Manages Dark/Light/System color modes and provides a Palette with colors
    /// for general UI, piano roll, and phoneme editor components.
    /// Optionally persists the selected mode via AppSettings.
    class Theme : public QObject {
        Q_OBJECT

    public:
        /// @brief Theme color mode.
        enum Mode {
            Dark,         ///< Dark color scheme.
            Light,        ///< Light color scheme.
            FollowSystem  ///< Follow the OS color scheme.
        };
        Q_ENUM(Mode)

        /// @brief Complete color palette for the application.
        struct Palette {
            QColor bgPrimary;     ///< Primary background color.
            QColor bgSecondary;   ///< Secondary background color.
            QColor bgSurface;     ///< Surface/card background color.
            QColor bgHover;       ///< Hover state background color.
            QColor accent;        ///< Accent/highlight color.
            QColor textPrimary;   ///< Primary text color.
            QColor textSecondary; ///< Secondary/muted text color.
            QColor textDisabled;  ///< Disabled text color.
            QColor border;        ///< Standard border color.
            QColor borderLight;   ///< Light/subtle border color.
            QColor success;       ///< Success indicator color.
            QColor error;         ///< Error indicator color.
            QColor warning;       ///< Warning indicator color.

            /// @brief Piano roll editor colors.
            struct {
                QColor background;          ///< Piano roll background.
                QColor gridSemitone;        ///< Semitone grid line color.
                QColor gridCents;           ///< Cents subdivision grid color.
                QColor barLine;             ///< Bar/measure line color.
                QColor rulerBg;             ///< Time ruler background.
                QColor rulerTick;           ///< Time ruler tick mark color.
                QColor rulerText;           ///< Time ruler text color.
                QColor pianoWhiteKey;       ///< White piano key color.
                QColor pianoBlackKey;       ///< Black piano key color.
                QColor pianoBg;             ///< Piano keyboard background.
                QColor noteDefault;         ///< Default note fill color.
                QColor noteDefaultTop;      ///< Default note top gradient.
                QColor noteDefaultBottom;   ///< Default note bottom gradient.
                QColor noteBorder;          ///< Note border color.
                QColor noteSlur;            ///< Slur note fill color.
                QColor noteSlurBorder;      ///< Slur note border color.
                QColor noteRestFill;        ///< Rest note fill color.
                QColor noteRestBorder;      ///< Rest note border color.
                QColor noteText;            ///< Note label text color.
                QColor noteSelectedTop;     ///< Selected note top gradient.
                QColor noteSelectedBottom;  ///< Selected note bottom gradient.
                QColor noteSelectedBorder;  ///< Selected note border color.
                QColor noteSelectedGlow;    ///< Selected note glow effect color.
                QColor f0Default;           ///< F0 curve default color.
                QColor f0Dimmed;            ///< F0 curve dimmed/inactive color.
                QColor f0Selected;          ///< F0 curve selected segment color.
                QColor snapGuide;           ///< Snap-to-grid guide line color.
                QColor rubberBandBorder;    ///< Rubber band selection border.
                QColor rubberBandFill;      ///< Rubber band selection fill.
                QColor playhead;            ///< Playhead line color (playing).
                QColor playheadIdle;        ///< Playhead line color (stopped).
            } pianoRoll;

            /// @brief Phoneme editor tier/boundary colors.
            struct {
                QColor tierBackground;       ///< Active tier background.
                QColor tierBackgroundInactive; ///< Inactive tier background.
                QColor intervalFillA;        ///< Alternating interval fill A.
                QColor intervalFillB;        ///< Alternating interval fill B.
                QColor intervalBorder;       ///< Interval border color.
                QColor boundaryNormal;       ///< Boundary line normal state.
                QColor boundaryHovered;      ///< Boundary line hovered state.
                QColor boundaryDragged;      ///< Boundary line while dragging.
                QColor boundaryBindingLine;  ///< Cross-tier binding line color.
                QColor labelText;            ///< Phoneme label text color.
                QColor selectionBorder;      ///< Selection rectangle border.
            } phonemeEditor;
        };

        /// @brief Get the global Theme singleton.
        /// @return Reference to the singleton instance.
        static Theme &instance();
        /// @brief Replace the global Theme singleton.
        /// @param theme New Theme instance (ownership transferred).
        static void setInstance(std::unique_ptr<Theme> theme);
        /// @brief Reset the global Theme singleton to default.
        static void resetInstance();

        ~Theme() override = default;

        /// @brief Initialize the theme system with an application and optional settings persistence.
        /// @param app Reference to the QApplication.
        /// @param defaultMode Initial color mode.
        /// @param settings Optional AppSettings for persisting the theme choice.
        void init(QApplication &app, Mode defaultMode = Dark, dstools::AppSettings *settings = nullptr);

        /// @brief Switch to a new color mode.
        /// @param mode Target mode.
        void setMode(Mode mode);

        /// @brief Return the current color mode.
        /// @return Current Mode value.
        Mode mode() const;

        /// @brief Check whether the effective theme is dark.
        /// @return True if the current appearance is dark.
        bool isDark() const;

        /// @brief Return the current color palette.
        /// @return Const reference to the active Palette.
        const Palette &palette() const;

        /// @brief Populate a QMenu with theme mode radio actions.
        /// @param menu Target menu to populate.
        void populateThemeMenu(QMenu *menu);

    signals:
        /// @brief Emitted after the theme colors change.
        void themeChanged();

    private:
        explicit Theme(QObject *parent = nullptr);

        void applyTheme(bool dark);
        bool resolveIsDark() const;
        void onSystemSchemeChanged();

        QApplication *m_app = nullptr;
        dstools::AppSettings *m_settings = nullptr;
        Mode m_mode = Dark;
        bool m_isDark = true;
        Palette m_palette;
        QActionGroup *m_actionGroup = nullptr;
    };

} // namespace dsfw
