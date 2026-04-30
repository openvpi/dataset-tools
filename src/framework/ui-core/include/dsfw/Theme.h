#pragma once

#include <dsfw/AppSettings.h>

#include <QApplication>
#include <QColor>
#include <memory>
#include <QObject>
#include <QPalette>

class QActionGroup;
class QMenu;

namespace dsfw {

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

        struct {
            QColor background;
            QColor gridSemitone;
            QColor gridCents;
            QColor barLine;
            QColor rulerBg;
            QColor rulerTick;
            QColor rulerText;
            QColor pianoWhiteKey;
            QColor pianoBlackKey;
            QColor pianoBg;
            QColor noteDefault;
            QColor noteDefaultTop;
            QColor noteDefaultBottom;
            QColor noteBorder;
            QColor noteSlur;
            QColor noteSlurBorder;
            QColor noteRestFill;
            QColor noteRestBorder;
            QColor noteText;
            QColor noteSelectedTop;
            QColor noteSelectedBottom;
            QColor noteSelectedBorder;
            QColor noteSelectedGlow;
            QColor f0Default;
            QColor f0Dimmed;
            QColor f0Selected;
            QColor snapGuide;
            QColor rubberBandBorder;
            QColor rubberBandFill;
            QColor playhead;
            QColor playheadIdle;
        } pianoRoll;

        struct {
            QColor tierBackground;
            QColor tierBackgroundInactive;
            QColor intervalFillA;
            QColor intervalFillB;
            QColor intervalBorder;
            QColor boundaryNormal;
            QColor boundaryHovered;
            QColor boundaryDragged;
            QColor boundaryBindingLine;
            QColor labelText;
            QColor selectionBorder;
        } phonemeEditor;
    };

    static Theme &instance();
    static void setInstance(std::unique_ptr<Theme> theme);
    static void resetInstance();

    ~Theme() override = default;

    void init(QApplication &app, Mode defaultMode = Dark, AppSettings *settings = nullptr);

    void setMode(Mode mode);

    Mode mode() const;

    bool isDark() const;

    const Palette &palette() const;

    void populateThemeMenu(QMenu *menu);

signals:
    void themeChanged();

private:
    explicit Theme(QObject *parent = nullptr);

    void applyTheme(bool dark);
    bool resolveIsDark() const;
    void onSystemSchemeChanged();

    QApplication *m_app = nullptr;
    AppSettings *m_settings = nullptr;
    Mode m_mode = Dark;
    bool m_isDark = true;
    Palette m_palette;
    QActionGroup *m_actionGroup = nullptr;
};

} // namespace dsfw
