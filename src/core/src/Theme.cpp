#include <dstools/Theme.h>

#include <dstools/AppSettings.h>
#include <dstools/CommonKeys.h>

#include <QActionGroup>
#include <QDebug>
#include <QFile>
#include <QGuiApplication>
#include <QMenu>
#include <QStyleFactory>
#include <QStyleHints>

namespace dstools {

// ── Palette definitions (JetBrains New UI inspired) ──────────────────

static Theme::Palette makeDarkPalette() {
    Theme::Palette p;
    p.bgPrimary    = QColor(0x1E, 0x1F, 0x22); // #1E1F22
    p.bgSecondary  = QColor(0x2B, 0x2D, 0x30); // #2B2D30
    p.bgSurface    = QColor(0x39, 0x3B, 0x40); // #393B40
    p.bgHover      = QColor(0x39, 0x3B, 0x40); // #393B40
    p.accent       = QColor(0x35, 0x74, 0xF0); // #3574F0
    p.textPrimary  = QColor(0xDF, 0xE1, 0xE5); // #DFE1E5
    p.textSecondary= QColor(0x6F, 0x73, 0x7A); // #6F737A
    p.textDisabled = QColor(0x5A, 0x5D, 0x63); // #5A5D63
    p.border       = QColor(0x43, 0x45, 0x4A); // #43454A
    p.borderLight  = QColor(0x4E, 0x51, 0x57); // #4E5157
    p.success      = QColor(0x57, 0x96, 0x5C); // #57965C
    p.error        = QColor(0xDB, 0x5C, 0x5C); // #DB5C5C
    p.warning      = QColor(0xD6, 0xAE, 0x58); // #D6AE58
    return p;
}

static Theme::Palette makeLightPalette() {
    Theme::Palette p;
    p.bgPrimary    = QColor(0xFF, 0xFF, 0xFF); // #FFFFFF
    p.bgSecondary  = QColor(0xF7, 0xF8, 0xFA); // #F7F8FA
    p.bgSurface    = QColor(0xEB, 0xEC, 0xF0); // #EBECF0
    p.bgHover      = QColor(0xED, 0xF3, 0xFF); // #EDF3FF
    p.accent       = QColor(0x35, 0x74, 0xF0); // #3574F0
    p.textPrimary  = QColor(0x00, 0x00, 0x00); // #000000
    p.textSecondary= QColor(0x81, 0x85, 0x94); // #818594
    p.textDisabled = QColor(0xA8, 0xAD, 0xBD); // #A8ADBD
    p.border       = QColor(0xEB, 0xEC, 0xF0); // #EBECF0
    p.borderLight  = QColor(0xC9, 0xCC, 0xD6); // #C9CCD6
    p.success      = QColor(0x20, 0x8A, 0x3C); // #208A3C
    p.error        = QColor(0xDB, 0x3B, 0x4B); // #DB3B4B
    p.warning      = QColor(0xA4, 0x67, 0x04); // #A46704
    return p;
}

// ── Singleton ────────────────────────────────────────────────────────

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

Theme::Theme(QObject *parent) : QObject(parent) {}

// ── Public API ───────────────────────────────────────────────────────

void Theme::init(QApplication &app, Mode defaultMode, AppSettings *settings) {
    m_app = &app;
    m_settings = settings;

    if (m_settings) {
        m_mode = static_cast<Mode>(m_settings->get(CommonKeys::ThemeMode));
    } else {
        m_mode = defaultMode;
    }

    // Listen for system theme changes
    if (auto *hints = QGuiApplication::styleHints()) {
        connect(hints, &QStyleHints::colorSchemeChanged,
                this, &Theme::onSystemSchemeChanged);
    }

    applyTheme(resolveIsDark());
}

void Theme::setMode(Mode mode) {
    if (m_mode == mode)
        return;
    m_mode = mode;

    if (m_settings) {
        m_settings->set(CommonKeys::ThemeMode, static_cast<int>(mode));
    }

    applyTheme(resolveIsDark());

    // Update action group if it exists
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
        {tr("Dark"),          Dark},
        {tr("Light"),         Light},
        {tr("Follow System"), FollowSystem},
    };

    for (const auto &e : entries) {
        auto *action = themeMenu->addAction(e.label);
        action->setCheckable(true);
        action->setData(static_cast<int>(e.mode));
        action->setChecked(e.mode == m_mode);
        m_actionGroup->addAction(action);
        connect(action, &QAction::triggered, this, [this, mode = e.mode]() {
            setMode(mode);
        });
    }
}

// ── Private ──────────────────────────────────────────────────────────

bool Theme::resolveIsDark() const {
    if (m_mode == Dark)
        return true;
    if (m_mode == Light)
        return false;

    // FollowSystem
    if (auto *hints = QGuiApplication::styleHints()) {
        auto scheme = hints->colorScheme();
        if (scheme == Qt::ColorScheme::Light)
            return false;
        if (scheme == Qt::ColorScheme::Dark)
            return true;
    }
    // Unknown / unsupported → default dark
    return true;
}

void Theme::applyTheme(bool dark) {
    m_isDark = dark;
    m_palette = dark ? makeDarkPalette() : makeLightPalette();

    if (!m_app)
        return;

    // Fusion is required for QSS to render correctly on Windows.
    // Native styles (windowsvista) don't fully respect QSS, causing
    // mixed native/custom painting (black-white artifacts on Win10).
    m_app->setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    // Load QSS
    QString qssPath = dark ? QStringLiteral(":/themes/dark.qss")
                           : QStringLiteral(":/themes/light.qss");
    QFile qssFile(qssPath);
    if (qssFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_app->setStyleSheet(QString::fromUtf8(qssFile.readAll()));
        qssFile.close();
    } else {
        qWarning() << "Theme: failed to load QSS from" << qssPath;
        m_app->setStyleSheet({});
    }

    // QPalette fallback for widgets not covered by QSS
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
    // setPalette AFTER setStyle — setStyle can reset palette
    m_app->setPalette(pal);

    emit themeChanged();
}

void Theme::onSystemSchemeChanged() {
    if (m_mode != FollowSystem)
        return;
    applyTheme(resolveIsDark());
}

} // namespace dstools
