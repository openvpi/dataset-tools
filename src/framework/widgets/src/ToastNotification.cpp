#include <dsfw/widgets/ToastNotification.h>

#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPropertyAnimation>
#include <QTimer>

namespace dsfw::widgets {

    static constexpr int kToastWidth = 400;
    static constexpr int kToastSpacing = 8;
    static constexpr int kBottomMargin = 24;
    static constexpr int kSlideInDuration = 200;
    static constexpr int kFadeOutDuration = 300;

    static QString styleForType(ToastType type) {
        QString bg;
        switch (type) {
            case ToastType::Info:
                bg = QStringLiteral("rgba(40, 55, 85, 230)");
                break;
            case ToastType::Warning:
                bg = QStringLiteral("rgba(160, 90, 10, 230)");
                break;
            case ToastType::Error:
                bg = QStringLiteral("rgba(150, 30, 30, 230)");
                break;
        }
        return QStringLiteral(
            "QFrame {"
            "  background: %1;"
            "  color: white;"
            "  border-radius: 8px;"
            "  padding: 12px;"
            "  font-size: 13px;"
            "}")
            .arg(bg);
    }

    ToastNotification::ToastNotification(QWidget *parent, ToastType type,
                                         const QString &message, int timeoutMs)
        : QFrame(parent), m_type(type) {
        setFrameShape(QFrame::NoFrame);
        setStyleSheet(styleForType(type));
        setFixedWidth(kToastWidth);

        auto *layout = new QHBoxLayout(this);
        layout->setContentsMargins(12, 8, 12, 8);

        m_label = new QLabel(message, this);
        m_label->setWordWrap(true);
        m_label->setStyleSheet(QStringLiteral("background: transparent; color: white;"));
        layout->addWidget(m_label);

        adjustSize();

        // Install event filter on parent for resize tracking.
        if (parent)
            parent->installEventFilter(this);

        // Auto-dismiss timer.
        m_timer = new QTimer(this);
        m_timer->setSingleShot(true);
        m_timer->setInterval(timeoutMs);
        connect(m_timer, &QTimer::timeout, this, &ToastNotification::startDismiss);
    }

    ToastNotification::~ToastNotification() {
        if (auto *p = parentWidget())
            p->removeEventFilter(this);
    }

    void ToastNotification::show(QWidget *parent, ToastType type,
                                 const QString &message, int timeoutMs) {
        if (!parent)
            return;

        auto *toast = new ToastNotification(parent, type, message, timeoutMs);

        // Calculate start (off-screen) and end positions.
        toast->reposition();
        const QPoint endPos = toast->pos();
        const QPoint startPos(endPos.x(), parent->height());

        toast->move(startPos);
        toast->QFrame::show();
        toast->raise();

        // Slide-in animation.
        auto *slideIn = new QPropertyAnimation(toast, "pos", toast);
        slideIn->setDuration(kSlideInDuration);
        slideIn->setStartValue(startPos);
        slideIn->setEndValue(endPos);
        slideIn->setEasingCurve(QEasingCurve::OutCubic);
        slideIn->start(QAbstractAnimation::DeleteWhenStopped);

        toast->m_timer->start();
    }

    qreal ToastNotification::opacity() const {
        return m_opacity;
    }

    void ToastNotification::setOpacity(qreal opacity) {
        m_opacity = opacity;
        update();
    }

    void ToastNotification::paintEvent(QPaintEvent *event) {
        QPainter painter(this);
        painter.setOpacity(m_opacity);
        painter.setRenderHint(QPainter::Antialiasing);
        QFrame::paintEvent(event);
    }

    bool ToastNotification::eventFilter(QObject *watched, QEvent *event) {
        if (watched == parentWidget() && event->type() == QEvent::Resize) {
            reposition();
        }
        return QFrame::eventFilter(watched, event);
    }

    void ToastNotification::startDismiss() {
        auto *fadeOut = new QPropertyAnimation(this, "opacity", this);
        fadeOut->setDuration(kFadeOutDuration);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        fadeOut->setEasingCurve(QEasingCurve::InCubic);
        connect(fadeOut, &QPropertyAnimation::finished, this, [this]() {
            // After removal, reposition remaining toasts.
            auto *p = parentWidget();
            deleteLater();
            if (p) {
                // Defer reposition so deleteLater has taken effect.
                QTimer::singleShot(0, p, [p]() {
                    const auto toasts = p->findChildren<ToastNotification *>(
                        QString(), Qt::FindDirectChildrenOnly);
                    for (auto *t : toasts)
                        t->reposition();
                });
            }
        });
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void ToastNotification::reposition() {
        auto *p = parentWidget();
        if (!p)
            return;

        // Count toasts below this one (those created before this one that are
        // still alive). We use the sibling order from findChildren.
        const auto siblings = p->findChildren<ToastNotification *>(
            QString(), Qt::FindDirectChildrenOnly);

        int indexFromBottom = 0;
        for (const auto *s : siblings) {
            if (s == this)
                break;
            ++indexFromBottom;
        }

        const int x = (p->width() - width()) / 2;
        const int y = p->height() - kBottomMargin - height()
                      - indexFromBottom * (height() + kToastSpacing);
        move(x, y);
    }

} // namespace dsfw::widgets
