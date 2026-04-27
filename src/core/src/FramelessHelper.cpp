#include <dstools/FramelessHelper.h>
#include <dstools/Theme.h>

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QToolButton>

#include <QWKWidgets/widgetwindowagent.h>

namespace dstools {

// ── Title-bar button with fixed size and theme-aware colors ─────────

class TitleBarButton : public QToolButton {
public:
    enum Role { Minimize, Maximize, Close };

    explicit TitleBarButton(Role role, QWidget *parent = nullptr)
        : QToolButton(parent), m_role(role) {
        setFixedSize(46, 32);
        setAutoRaise(true);
        setFocusPolicy(Qt::NoFocus);
        updateIcon();
    }

    void setMaximized(bool maximized) {
        m_maximized = maximized;
        updateIcon();
    }

private:
    void updateIcon() {
        switch (m_role) {
            case Minimize:
                setText(QStringLiteral("\u2500")); // ─
                break;
            case Maximize:
                setText(m_maximized ? QStringLiteral("\u2750")   // ❐
                                   : QStringLiteral("\u25A1"));  // □
                break;
            case Close:
                setText(QStringLiteral("\u2715")); // ✕
                break;
        }
    }

    Role m_role;
    bool m_maximized = false;
};

// ── Title bar widget ────────────────────────────────────────────────

class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QMainWindow *window, QWidget *parent = nullptr)
        : QWidget(parent), m_window(window) {
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        // Title label
        m_titleLabel = new QLabel(this);
        m_titleLabel->setContentsMargins(10, 0, 0, 0);
        layout->addWidget(m_titleLabel);

        layout->addStretch();

        // System buttons
        m_minBtn = new TitleBarButton(TitleBarButton::Minimize, this);
        m_maxBtn = new TitleBarButton(TitleBarButton::Maximize, this);
        m_closeBtn = new TitleBarButton(TitleBarButton::Close, this);

        layout->addWidget(m_minBtn);
        layout->addWidget(m_maxBtn);
        layout->addWidget(m_closeBtn);

        connect(m_minBtn, &QToolButton::clicked, window, &QWidget::showMinimized);
        connect(m_maxBtn, &QToolButton::clicked, this, [this]() {
            if (m_window->isMaximized())
                m_window->showNormal();
            else
                m_window->showMaximized();
        });
        connect(m_closeBtn, &QToolButton::clicked, window, &QWidget::close);

        setFixedHeight(32);
        setAutoFillBackground(true);
        updateTitle();
        applyTheme();

        connect(&Theme::instance(), &Theme::themeChanged, this, &TitleBar::applyTheme);
    }

    void updateTitle() {
        m_titleLabel->setText(m_window->windowTitle());
    }

    void updateMaximizeButton() {
        m_maxBtn->setMaximized(m_window->isMaximized());
    }

    TitleBarButton *minimizeButton() const { return m_minBtn; }
    TitleBarButton *maximizeButton() const { return m_maxBtn; }
    TitleBarButton *closeButton() const { return m_closeBtn; }

private:
    void applyTheme() {
        const auto &pal = Theme::instance().palette();

        // Title bar background via palette
        auto widgetPal = palette();
        widgetPal.setColor(QPalette::Window, pal.bgPrimary);
        setPalette(widgetPal);

        // Title label
        m_titleLabel->setStyleSheet(QStringLiteral(
            "QLabel { color: %1; background: transparent; font-size: 12px; }"
        ).arg(pal.textPrimary.name()));

        // Button style
        const QString btnStyle = QStringLiteral(
            "QToolButton { color: %1; background: transparent; border: none; font-size: 13px; }"
            "QToolButton:hover { background: %2; }"
        ).arg(pal.textPrimary.name(), pal.bgHover.name());

        m_minBtn->setStyleSheet(btnStyle);
        m_maxBtn->setStyleSheet(btnStyle);

        // Close button gets red hover
        m_closeBtn->setStyleSheet(QStringLiteral(
            "QToolButton { color: %1; background: transparent; border: none; font-size: 13px; }"
            "QToolButton:hover { background: #E81123; color: white; }"
        ).arg(pal.textPrimary.name()));
    }

    QMainWindow *m_window;
    QLabel *m_titleLabel;
    TitleBarButton *m_minBtn;
    TitleBarButton *m_maxBtn;
    TitleBarButton *m_closeBtn;
};

// ── Event filter to track window state changes ──────────────────────

class WindowStateFilter : public QObject {
public:
    WindowStateFilter(QObject *parent, TitleBar *titleBar)
        : QObject(parent), m_titleBar(titleBar) {}

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::WindowStateChange)
            m_titleBar->updateMaximizeButton();
        return QObject::eventFilter(obj, event);
    }

private:
    TitleBar *m_titleBar;
};

// ── FramelessHelper implementation ──────────────────────────────────

void FramelessHelper::apply(QMainWindow *window) {
    if (!window)
        return;

    // Prevent native ancestor creation issues
    window->setAttribute(Qt::WA_DontCreateNativeAncestors);

    // Create and attach the window agent
    auto *agent = new QWK::WidgetWindowAgent(window);
    agent->setup(window);

    // Create title bar
    auto *titleBar = new TitleBar(window);

    // If window has a menu bar, embed it in the title bar
    if (auto *menuBar = window->menuBar(); menuBar && !menuBar->actions().isEmpty()) {
        auto *layout = qobject_cast<QHBoxLayout *>(titleBar->layout());
        if (layout) {
            // Insert after title label (index 1), before stretch
            layout->insertWidget(1, menuBar);
            menuBar->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
            // Mark menu bar as hit-test visible so clicks go to it
            agent->setHitTestVisible(menuBar, true);
        }
    }

    // Register with QWindowKit
    agent->setTitleBar(titleBar);
    agent->setSystemButton(QWK::WindowAgentBase::Minimize, titleBar->minimizeButton());
    agent->setSystemButton(QWK::WindowAgentBase::Maximize, titleBar->maximizeButton());
    agent->setSystemButton(QWK::WindowAgentBase::Close, titleBar->closeButton());

    // Place title bar at top of window (replaces menu widget slot)
    window->setMenuWidget(titleBar);

    // Keep title synced
    QObject::connect(window, &QWidget::windowTitleChanged, titleBar, &TitleBar::updateTitle);

    // Track maximize state for button icon
    window->installEventFilter(new WindowStateFilter(window, titleBar));
}

} // namespace dstools

#include "FramelessHelper.moc"
