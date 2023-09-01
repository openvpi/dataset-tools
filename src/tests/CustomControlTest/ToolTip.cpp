//
// Created by fluty on 2023/8/30.
//

#include <QDebug>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QApplication>
#include <QCursor>
#include <QScreen>
#include <QGraphicsDropShadowEffect>

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
}

ToolTipFilter::ToolTipFilter(QWidget *parent, int showDelay, const ToolTip::ToolTipPosition &position) {
    m_parent = parent;
    parent->setAttribute(Qt::WA_Hover, true);
//    m_tooltip = new ToolTip("", parent);
}

bool ToolTipFilter::eventFilter(QObject *object, QEvent *event) {
    auto type = event->type();
    if (type == QEvent::ToolTip)
        return true; // discard the original QToolTip event
    else if (type == QEvent::Hide || type == QEvent::Leave) {
//        qDebug() << "Hide Tool Tip";
        m_tooltip->hide();
    } else if (type == QEvent::Enter) {
//        qDebug() << "Show Tool Tip";
        if (m_tooltip != nullptr)
            m_tooltip = new ToolTip(m_parent->toolTip(), m_parent);
        auto pos = QCursor::pos();
//        qDebug() << pos;
        m_tooltip->move(pos.x(), pos.y());
        m_tooltip->show();
    } else if (type == QEvent::MouseButtonPress) {
//        qDebug() << "Hide Tool Tip";
        m_tooltip->hide();
    } else if (type == QEvent::HoverMove) {
//        qDebug() << "Hover Move";
        if (m_tooltip != nullptr) {
            auto pos = QCursor::pos();
            m_tooltip->move(pos.x(), pos.y());
        }
    }

    return QObject::eventFilter(object, event);
}

ToolTipFilter::~ToolTipFilter() {
    delete m_tooltip;
}
