#include <dsfw/Theme.h>

#include <dsfw/AppSettings.h>
#include <dsfw/Log.h>

#include <QActionGroup>
#include <QFile>
#include <QGuiApplication>
#include <QMenu>
#include <QStyleFactory>
#include <QStyleHints>

namespace dsfw {

    static const dstools::SettingsKey<int> kThemeModeKey("General/themeMode", 0);

    static Theme::Palette makeDarkPalette() {
        Theme::Palette p;
        p.bgPrimary = QColor(0x1E, 0x1F, 0x22);
        p.bgSecondary = QColor(0x2B, 0x2D, 0x30);
        p.bgSurface = QColor(0x39, 0x3B, 0x40);
        p.bgHover = QColor(0x39, 0x3B, 0x40);
        p.accent = QColor(0x35, 0x74, 0xF0);
        p.textPrimary = QColor(0xDF, 0xE1, 0xE5);
        p.textSecondary = QColor(0x6F, 0x73, 0x7A);
        p.textDisabled = QColor(0x5A, 0x5D, 0x63);
        p.border = QColor(0x43, 0x45, 0x4A);
        p.borderLight = QColor(0x4E, 0x51, 0x57);
        p.success = QColor(0x57, 0x96, 0x5C);
        p.error = QColor(0xDB, 0x5C, 0x5C);
        p.warning = QColor(0xD6, 0xAE, 0x58);

        p.pianoRoll.background = QColor("#1A1A22");
        p.pianoRoll.gridSemitone = QColor("#26262E");
        p.pianoRoll.gridCents = QColor("#1F1F28");
        p.pianoRoll.barLine = QColor("#3A3A44");
        p.pianoRoll.rulerBg = QColor("#22222C");
        p.pianoRoll.rulerTick = QColor("#555555");
        p.pianoRoll.rulerText = QColor("#9898A8");
        p.pianoRoll.pianoWhiteKey = QColor("#E8E8E8");
        p.pianoRoll.pianoBlackKey = QColor("#2A2A2A");
        p.pianoRoll.pianoBg = QColor("#22222C");
        p.pianoRoll.noteDefault = QColor("#4A8FD9");
        p.pianoRoll.noteDefaultTop = QColor("#5A9FE9");
        p.pianoRoll.noteDefaultBottom = QColor("#4485C9");
        p.pianoRoll.noteBorder = QColor("#5EA2ED");
        p.pianoRoll.noteSlur = QColor("#2E6BB0");
        p.pianoRoll.noteSlurBorder = QColor("#4A8FCC");
        p.pianoRoll.noteRestFill = QColor(0x3A, 0x3A, 0x44, int(0.35 * 255));
        p.pianoRoll.noteRestBorder = QColor(0x5A, 0x5A, 0x64, int(0.5 * 255));
        p.pianoRoll.noteText = QColor("#FFFFFF");
        p.pianoRoll.noteSelectedTop = QColor("#FFD066");
        p.pianoRoll.noteSelectedBottom = QColor("#E8B040");
        p.pianoRoll.noteSelectedBorder = QColor("#FFE088");
        p.pianoRoll.noteSelectedGlow = QColor(0xFF, 0xD0, 0x66, 60);
        p.pianoRoll.f0Default = QColor("#FF8C42");
        p.pianoRoll.f0Dimmed = QColor(0xFF, 0x8C, 0x42, int(0.4 * 255));
        p.pianoRoll.f0Selected = QColor("#FFB366");
        p.pianoRoll.snapGuide = QColor("#FFD066");
        p.pianoRoll.rubberBandBorder = QColor(0x4A, 0x8F, 0xD9, 180);
        p.pianoRoll.rubberBandFill = QColor(0x4A, 0x8F, 0xD9, 38);
        p.pianoRoll.playhead = QColor("#EF5350");
        p.pianoRoll.playheadIdle = QColor("#F0F1F2");

        p.phonemeEditor.tierBackground = QColor(50, 50, 60);
        p.phonemeEditor.tierBackgroundInactive = QColor(40, 40, 45);
        p.phonemeEditor.intervalFillA = QColor(70, 100, 140);
        p.phonemeEditor.intervalFillB = QColor(60, 90, 130);
        p.phonemeEditor.intervalBorder = QColor(100, 140, 200);
        p.phonemeEditor.boundaryNormal = QColor(180, 180, 200, 180);
        p.phonemeEditor.boundaryHovered = QColor(255, 255, 255);
        p.phonemeEditor.boundaryDragged = QColor(255, 200, 100);
        p.phonemeEditor.boundaryBindingLine = QColor(255, 200, 100, 120);
        p.phonemeEditor.labelText = QColor(255, 255, 255);
        p.phonemeEditor.selectionBorder = QColor(255, 255, 0);

        return p;
    }

    static Theme::Palette makeLightPalette() {
        Theme::Palette p;
        p.bgPrimary = QColor(0xFF, 0xFF, 0xFF);
        p.bgSecondary = QColor(0xF7, 0xF8, 0xFA);
        p.bgSurface = QColor(0xEB, 0xEC, 0xF0);
        p.bgHover = QColor(0xED, 0xF3, 0xFF);
        p.accent = QColor(0x35, 0x74, 0xF0);
        p.textPrimary = QColor(0x00, 0x00, 0x00);
        p.textSecondary = QColor(0x81, 0x85, 0x94);
        p.textDisabled = QColor(0xA8, 0xAD, 0xBD);
        p.border = QColor(0xEB, 0xEC, 0xF0);
        p.borderLight = QColor(0xC9, 0xCC, 0xD6);
        p.success = QColor(0x20, 0x8A, 0x3C);
        p.error = QColor(0xDB, 0x3B, 0x4B);
        p.warning = QColor(0xA4, 0x67, 0x04);

        p.pianoRoll.background = QColor("#F0F0F5");
        p.pianoRoll.gridSemitone = QColor("#E0E0E8");
        p.pianoRoll.gridCents = QColor("#E8E8EE");
        p.pianoRoll.barLine = QColor("#C0C0CC");
        p.pianoRoll.rulerBg = QColor("#E4E4EC");
        p.pianoRoll.rulerTick = QColor("#999999");
        p.pianoRoll.rulerText = QColor("#555566");
        p.pianoRoll.pianoWhiteKey = QColor("#FFFFFF");
        p.pianoRoll.pianoBlackKey = QColor("#3A3A3A");
        p.pianoRoll.pianoBg = QColor("#E4E4EC");
        p.pianoRoll.noteDefault = QColor("#3A7CC6");
        p.pianoRoll.noteDefaultTop = QColor("#4A8CD6");
        p.pianoRoll.noteDefaultBottom = QColor("#3272B6");
        p.pianoRoll.noteBorder = QColor("#2E6AAE");
        p.pianoRoll.noteSlur = QColor("#6BAAE0");
        p.pianoRoll.noteSlurBorder = QColor("#5090C0");
        p.pianoRoll.noteRestFill = QColor(0xC0, 0xC0, 0xCC, int(0.35 * 255));
        p.pianoRoll.noteRestBorder = QColor(0xA0, 0xA0, 0xAA, int(0.5 * 255));
        p.pianoRoll.noteText = QColor("#FFFFFF");
        p.pianoRoll.noteSelectedTop = QColor("#E8A020");
        p.pianoRoll.noteSelectedBottom = QColor("#D09018");
        p.pianoRoll.noteSelectedBorder = QColor("#C08010");
        p.pianoRoll.noteSelectedGlow = QColor(0xE8, 0xA0, 0x20, 60);
        p.pianoRoll.f0Default = QColor("#E07030");
        p.pianoRoll.f0Dimmed = QColor(0xE0, 0x70, 0x30, int(0.4 * 255));
        p.pianoRoll.f0Selected = QColor("#FF9050");
        p.pianoRoll.snapGuide = QColor("#E0A020");
        p.pianoRoll.rubberBandBorder = QColor(0x3A, 0x7C, 0xC6, 180);
        p.pianoRoll.rubberBandFill = QColor(0x3A, 0x7C, 0xC6, 38);
        p.pianoRoll.playhead = QColor("#E04040");
        p.pianoRoll.playheadIdle = QColor("#404050");

        p.phonemeEditor.tierBackground = QColor(220, 225, 235);
        p.phonemeEditor.tierBackgroundInactive = QColor(235, 237, 242);
        p.phonemeEditor.intervalFillA = QColor(170, 200, 240);
        p.phonemeEditor.intervalFillB = QColor(155, 185, 225);
        p.phonemeEditor.intervalBorder = QColor(90, 130, 190);
        p.phonemeEditor.boundaryNormal = QColor(100, 100, 120, 180);
        p.phonemeEditor.boundaryHovered = QColor(40, 40, 50);
        p.phonemeEditor.boundaryDragged = QColor(180, 120, 40);
        p.phonemeEditor.boundaryBindingLine = QColor(180, 120, 40, 120);
        p.phonemeEditor.labelText = QColor(0, 0, 0);
        p.phonemeEditor.selectionBorder = QColor(200, 150, 0);

        return p;
    }

    static std::unique_ptr<Theme> s_customInstance;

    Theme &Theme::instance() {
        if (s_customInstance)
            return *s_customInstance;
        static Theme s;
        return s;
    }

    void Theme::setInstance(std::unique_ptr<Theme> theme) {
        s_customInstance = std::move(theme);
    }

    void Theme::resetInstance() {
        s_customInstance.reset();
    }

    Theme::Theme(QObject *parent) : QObject(parent) {
    }

    void Theme::init(QApplication &app, Mode defaultMode, dstools::AppSettings *settings) {
        m_app = &app;
        m_settings = settings;

        if (m_settings) {
            m_mode = static_cast<Mode>(m_settings->get(kThemeModeKey));
        } else {
            m_mode = defaultMode;
        }

        if (auto *hints = QGuiApplication::styleHints()) {
            connect(hints, &QStyleHints::colorSchemeChanged, this, &Theme::onSystemSchemeChanged);
        }

        applyTheme(resolveIsDark());
    }

    void Theme::setMode(Mode mode) {
        if (m_mode == mode)
            return;
        m_mode = mode;

        if (m_settings) {
            m_settings->set(kThemeModeKey, static_cast<int>(mode));
        }

        applyTheme(resolveIsDark());

        if (m_actionGroup) {
            const auto actions = m_actionGroup->actions();
            for (auto *action : actions) {
                if (action->data().toInt() == static_cast<int>(mode)) {
                    action->setChecked(true);
                    break;
                }
            }
        }
    }

    Theme::Mode Theme::mode() const {
        return m_mode;
    }

    bool Theme::isDark() const {
        return m_isDark;
    }

    const Theme::Palette &Theme::palette() const {
        return m_palette;
    }

    void Theme::populateThemeMenu(QMenu *menu) {
        auto *themeMenu = menu->addMenu(tr("Theme"));

        if (!m_actionGroup)
            m_actionGroup = new QActionGroup(this);
        m_actionGroup->setExclusive(true);

        struct Entry {
            QString label;
            Mode mode;
        };
        const Entry entries[] = {
            {tr("Dark"),          Dark        },
            {tr("Light"),         Light       },
            {tr("Follow System"), FollowSystem},
        };

        for (const auto &e : entries) {
            auto *action = themeMenu->addAction(e.label);
            action->setCheckable(true);
            action->setData(static_cast<int>(e.mode));
            action->setChecked(e.mode == m_mode);
            m_actionGroup->addAction(action);
            connect(action, &QAction::triggered, this, [this, mode = e.mode]() { setMode(mode); });
        }
    }

    bool Theme::resolveIsDark() const {
        if (m_mode == Dark)
            return true;
        if (m_mode == Light)
            return false;

        if (auto *hints = QGuiApplication::styleHints()) {
            auto scheme = hints->colorScheme();
            if (scheme == Qt::ColorScheme::Light)
                return false;
            if (scheme == Qt::ColorScheme::Dark)
                return true;
        }
        return true;
    }

    void Theme::applyTheme(bool dark) {
        m_isDark = dark;
        m_palette = dark ? makeDarkPalette() : makeLightPalette();

        if (!m_app)
            return;

        m_app->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

        QString qssPath = dark ? QStringLiteral(":/themes/dark.qss") : QStringLiteral(":/themes/light.qss");
        QFile qssFile(qssPath);
        if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString qss = QString::fromUtf8(qssFile.readAll());
            qssFile.close();

            qss.replace(QStringLiteral("{{bgPrimary}}"), m_palette.bgPrimary.name());
            qss.replace(QStringLiteral("{{bgSecondary}}"), m_palette.bgSecondary.name());
            qss.replace(QStringLiteral("{{bgSurface}}"), m_palette.bgSurface.name());
            qss.replace(QStringLiteral("{{bgHover}}"), m_palette.bgHover.name());
            qss.replace(QStringLiteral("{{accent}}"), m_palette.accent.name());
            qss.replace(QStringLiteral("{{textPrimary}}"), m_palette.textPrimary.name());
            qss.replace(QStringLiteral("{{textSecondary}}"), m_palette.textSecondary.name());
            qss.replace(QStringLiteral("{{textDisabled}}"), m_palette.textDisabled.name());
            qss.replace(QStringLiteral("{{border}}"), m_palette.border.name());
            qss.replace(QStringLiteral("{{borderLight}}"), m_palette.borderLight.name());
            qss.replace(QStringLiteral("{{success}}"), m_palette.success.name());
            qss.replace(QStringLiteral("{{error}}"), m_palette.error.name());
            qss.replace(QStringLiteral("{{warning}}"), m_palette.warning.name());

            m_app->setStyleSheet(qss);
        } else {
            DSFW_LOG_WARN("ui", ("Theme: failed to load QSS from " + qssPath.toStdString()).c_str());
            m_app->setStyleSheet({});
        }

        QPalette pal;
        pal.setColor(QPalette::Window, m_palette.bgPrimary);
        pal.setColor(QPalette::WindowText, m_palette.textPrimary);
        pal.setColor(QPalette::Base, m_palette.bgSecondary);
        pal.setColor(QPalette::AlternateBase, m_palette.bgSurface);
        pal.setColor(QPalette::Text, m_palette.textPrimary);
        pal.setColor(QPalette::Button, m_palette.bgSurface);
        pal.setColor(QPalette::ButtonText, m_palette.textPrimary);
        pal.setColor(QPalette::Highlight, m_palette.accent);
        pal.setColor(QPalette::HighlightedText, Qt::white);
        pal.setColor(QPalette::ToolTipBase, m_palette.bgSurface);
        pal.setColor(QPalette::ToolTipText, m_palette.textPrimary);
        pal.setColor(QPalette::PlaceholderText, m_palette.textSecondary);
        pal.setColor(QPalette::Disabled, QPalette::Text, m_palette.textDisabled);
        pal.setColor(QPalette::Disabled, QPalette::ButtonText, m_palette.textDisabled);
        m_app->setPalette(pal);

        emit themeChanged();
    }

    void Theme::onSystemSchemeChanged() {
        if (m_mode != FollowSystem)
            return;
        applyTheme(resolveIsDark());
    }

} // namespace dsfw
