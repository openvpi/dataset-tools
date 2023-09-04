//
// Created by fluty on 2023/8/30.
//

#include <QApplication>
#include <QCursor>
#include <QDebug>
#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QRect>
#include <QScreen>

#include "ToolTip.h"

ToolTip::ToolTip(QString text, QWidget *parent) : QFrame(parent) {
    auto label = new QLabel(text);
    label->setAlignment(Qt::AlignCenter);

    auto layout = new QHBoxLayout;
    layout->addWidget(label);
    layout->setMargin(0);
    layout->setAlignment(Qt::AlignCenter);

    auto container = new QFrame;
    container->setObjectName("container");
    container->setLayout(layout);
    container->setContentsMargins(8, 4, 8, 4);
    container->setStyleSheet("QFrame#container {"
                             "background: #FFFFFF; "
                             "border: 1px solid #10000000; "
                             "border-radius: 6px; "
                             "font-size: 10pt }");

    auto mainLayout = new QHBoxLayout;
    mainLayout->addWidget(container);
    mainLayout->setMargin(16);
    setLayout(mainLayout);

    //    if (parent != nullptr) {
    //        auto win = parent->window();
    //        auto rect = QApplication::screenAt(QCursor::pos())->availableGeometry();
    //        auto pos = QCursor::pos();
    ////        auto pos = win->mapToGlobal(QPoint());
    //        qDebug() << pos;
    //        move(pos.x(), pos.y());
    //    }
    auto shadowEffect = new QGraphicsDropShadowEffect(this);
    shadowEffect->setBlurRadius(24);
    shadowEffect->setColor(QColor(0, 0, 0, 32));
    shadowEffect->setOffset(0, 4);
    container->setGraphicsEffect(shadowEffect);

    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setWindowOpacity(0);
}

ToolTipFilter::ToolTipFilter(QWidget *parent, int showDelay, const ToolTip::ToolTipPosition &position) {
    m_parent = parent;
    parent->setAttribute(Qt::WA_Hover, true);
    m_tooltip = new ToolTip(m_parent->toolTip(), parent);
    m_opacityAnimation = new QPropertyAnimation(m_tooltip, "windowOpacity");
    m_opacityAnimation->setDuration(150);
}

bool ToolTipFilter::eventFilter(QObject *object, QEvent *event) {
    auto type = event->type();
    if (type == QEvent::ToolTip)
        return true; // discard the original QToolTip event
    else if (type == QEvent::Hide || type == QEvent::Leave) {
        hideToolTip();
    } else if (type == QEvent::Enter) {
        adjustToolTipPos();
        showToolTip();
    } else if (type == QEvent::MouseButtonPress) {
        hideToolTip();
    } else if (type == QEvent::HoverMove) {
        adjustToolTipPos();
    }

    return QObject::eventFilter(object, event);
}

ToolTipFilter::~ToolTipFilter() {
    delete m_tooltip;
}

void ToolTipFilter::adjustToolTipPos() {
    if (m_tooltip == nullptr)
        return;

    auto getPos = [&]() {
        auto cursorPos = QCursor::pos();
        auto screen = QApplication::screenAt(cursorPos);
        auto screenRect = screen->availableGeometry();
        auto toolTipRect = m_tooltip->rect();
        auto left = screenRect.left();
        auto top = screenRect.top();
        auto width = screenRect.width() - toolTipRect.width();
        auto height = screenRect.height() - toolTipRect.height();
        auto availableRect = QRect(left, top, width, height);

        auto x = cursorPos.x();
        auto y = cursorPos.y();
//        if (!availableRect.contains(cursorPos)) {
//            qDebug() << "Out of available area";
//        }

        if (x < availableRect.left())
            x = availableRect.left();
        else if (x > availableRect.right())
            x = availableRect.right();

        if (y < availableRect.top())
            y = availableRect.top();
        else if (y > availableRect.bottom())
            y = availableRect.bottom();

        return QPoint(x, y);
    };

    m_tooltip->move(getPos());
}

void ToolTipFilter::showToolTip() {
    m_opacityAnimation->stop();
    m_opacityAnimation->setStartValue(m_tooltip->windowOpacity());
    m_opacityAnimation->setEndValue(1);
    m_opacityAnimation->start();
    m_tooltip->show();
}

void ToolTipFilter::hideToolTip() {
    m_opacityAnimation->stop();
    m_opacityAnimation->setStartValue(m_tooltip->windowOpacity());
    m_opacityAnimation->setEndValue(0);
    m_opacityAnimation->start();
}
