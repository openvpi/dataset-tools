#include <dsfw/FramelessHelper.h>
#include <dsfw/Theme.h>
#include <dsfw/Log.h>

#include <QEvent>
#include <QHBoxLayout>
#include <QResizeEvent>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QPointer>
#include <QToolButton>
#include <QDialog>

#include <QWKWidgets/widgetwindowagent.h>

namespace dsfw {

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
                setText(QStringLiteral("\u2500"));
                break;
            case Maximize:
                setText(m_maximized ? QStringLiteral("\u2750")
                                   : QStringLiteral("\u25A1"));
                break;
            case Close:
                setText(QStringLiteral("\u2715"));
                break;
        }
    }

    Role m_role;
    bool m_maximized = false;
};

class TitleBar : public QWidget {
    Q_OBJECT
public:
    explicit TitleBar(QMainWindow *window, QWidget *parent = nullptr)
        : QWidget(parent), m_window(window) {
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        layout->addStretch();

        m_minBtn = new TitleBarButton(TitleBarButton::Minimize, this);
        m_maxBtn = new TitleBarButton(TitleBarButton::Maximize, this);
        m_closeBtn = new TitleBarButton(TitleBarButton::Close, this);

        layout->addWidget(m_minBtn, 0, Qt::AlignTop);
        layout->addWidget(m_maxBtn, 0, Qt::AlignTop);
        layout->addWidget(m_closeBtn, 0, Qt::AlignTop);

        m_titleLabel = new QLabel(this);
        m_titleLabel->setAlignment(Qt::AlignCenter);
        m_titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
        m_titleLabel->setAttribute(Qt::WA_NoSystemBackground);
        m_titleLabel->setStyleSheet(QStringLiteral("background: transparent;"));

        connect(m_minBtn, &QToolButton::clicked, window, &QWidget::showMinimized);
        connect(m_maxBtn, &QToolButton::clicked, this, [this]() {
            if (m_window->isMaximized())
                m_window->showNormal();
            else
                m_window->showMaximized();
        });
        connect(m_closeBtn, &QToolButton::clicked, window, &QWidget::close);

        // Use minimum height instead of fixed so the title bar can grow
        // when QMenuBar wraps items to a second row on narrow windows.
        setMinimumHeight(32);
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

protected:
    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        m_titleLabel->setGeometry(0, 0, width(), height());
        m_titleLabel->lower(); // keep behind menuBar and buttons
    }

private:
    void applyTheme() {
        const auto &pal = Theme::instance().palette();

        auto widgetPal = palette();
        widgetPal.setColor(QPalette::Window, pal.bgPrimary);
        setPalette(widgetPal);

        m_titleLabel->setStyleSheet(QStringLiteral(
            "QLabel { color: %1; background: transparent; font-size: 12px; }"
        ).arg(pal.textPrimary.name()));

        const QString btnStyle = QStringLiteral(
            "QToolButton { color: %1; background: transparent; border: none; font-size: 13px; }"
            "QToolButton:hover { background: %2; }"
        ).arg(pal.textPrimary.name(), pal.bgHover.name());

        m_minBtn->setStyleSheet(btnStyle);
        m_maxBtn->setStyleSheet(btnStyle);

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

class WindowStateFilter : public QObject {
public:
    WindowStateFilter(QObject *parent, TitleBar *titleBar)
        : QObject(parent), m_titleBar(titleBar) {}

protected:
    bool eventFilter(QObject *obj, QEvent *event) override {
        if (event->type() == QEvent::WindowStateChange) {
            if (m_titleBar)
                m_titleBar->updateMaximizeButton();
        }
        return QObject::eventFilter(obj, event);
    }

private:
    QPointer<TitleBar> m_titleBar;
};

void FramelessHelper::apply(QMainWindow *window) {
    if (!window)
        return;

    window->setAttribute(Qt::WA_DontCreateNativeAncestors);

    auto *agent = new QWK::WidgetWindowAgent(window);
    if (!agent->setup(window)) {
        // QWindowKit setup failed — fall back to native title bar so the window still shows
        DSFW_LOG_WARN("ui", "FramelessHelper: QWindowKit setup failed, falling back to native frame");
        delete agent;
        return;
    }

    auto *titleBar = new TitleBar(window);

    if (auto *menuBar = window->menuBar()) {
        auto *layout = qobject_cast<QHBoxLayout *>(titleBar->layout());
        if (layout) {
            layout->insertWidget(0, menuBar);
            menuBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
            agent->setHitTestVisible(menuBar, true);
        }
    }

    agent->setTitleBar(titleBar);
    agent->setSystemButton(QWK::WindowAgentBase::Minimize, titleBar->minimizeButton());
    agent->setSystemButton(QWK::WindowAgentBase::Maximize, titleBar->maximizeButton());
    agent->setSystemButton(QWK::WindowAgentBase::Close, titleBar->closeButton());

    window->setMenuWidget(titleBar);

    QObject::connect(window, &QWidget::windowTitleChanged, titleBar, &TitleBar::updateTitle);

    window->installEventFilter(new WindowStateFilter(window, titleBar));
}

class DialogTitleBar : public QWidget {
    Q_OBJECT
public:
    explicit DialogTitleBar(QDialog *dialog, QWidget *parent = nullptr)
        : QWidget(parent), m_dialog(dialog) {
        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);

        m_titleLabel = new QLabel(this);
        m_titleLabel->setContentsMargins(10, 0, 0, 0);
        layout->addWidget(m_titleLabel);

        layout->addStretch();

        m_closeBtn = new TitleBarButton(TitleBarButton::Close, this);
        layout->addWidget(m_closeBtn);

        connect(m_closeBtn, &QToolButton::clicked, dialog, &QDialog::reject);

        setFixedHeight(32);
        setAutoFillBackground(true);
        updateTitle();
        applyTheme();

        connect(&Theme::instance(), &Theme::themeChanged, this, &DialogTitleBar::applyTheme);
    }

    void updateTitle() {
        m_titleLabel->setText(m_dialog->windowTitle());
    }

    TitleBarButton *closeButton() const { return m_closeBtn; }

private:
    void applyTheme() {
        const auto &pal = Theme::instance().palette();

        auto widgetPal = palette();
        widgetPal.setColor(QPalette::Window, pal.bgPrimary);
        setPalette(widgetPal);

        m_titleLabel->setStyleSheet(QStringLiteral(
            "QLabel { color: %1; background: transparent; font-size: 12px; }"
        ).arg(pal.textPrimary.name()));

        m_closeBtn->setStyleSheet(QStringLiteral(
            "QToolButton { color: %1; background: transparent; border: none; font-size: 13px; }"
            "QToolButton:hover { background: #E81123; color: white; }"
        ).arg(pal.textPrimary.name()));
    }

    QDialog *m_dialog;
    QLabel *m_titleLabel;
    TitleBarButton *m_closeBtn;
};

void FramelessHelper::applyToDialog(QDialog *dialog) {
    if (!dialog)
        return;

    dialog->setAttribute(Qt::WA_DontCreateNativeAncestors);

    auto *agent = new QWK::WidgetWindowAgent(dialog);
    if (!agent->setup(dialog)) {
        DSFW_LOG_WARN("ui", "FramelessHelper: QWindowKit setup failed for dialog, falling back to native frame");
        delete agent;
        return;
    }

    auto *titleBar = new DialogTitleBar(dialog);

    agent->setTitleBar(titleBar);
    agent->setSystemButton(QWK::WindowAgentBase::Close, titleBar->closeButton());

    auto *layout = new QVBoxLayout(dialog);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(titleBar);

    QObject::connect(dialog, &QWidget::windowTitleChanged, titleBar, &DialogTitleBar::updateTitle);
}

} // namespace dsfw

#include "FramelessHelper.moc"
