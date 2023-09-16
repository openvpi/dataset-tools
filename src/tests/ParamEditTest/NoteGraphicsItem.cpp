//
// Created by fluty on 2023/9/16.
//

#include <QAction>
#include <QDebug>
#include <QPainter>
#include <QMenu>
#include <QGraphicsSceneContextMenuEvent>

#include "NoteGraphicsItem.h"

void NoteGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    QColor colorPrimary = QColor(112, 156, 255);
    auto scaleX = painter->transform().m11();
    auto scaleY = painter->transform().m22();

    QPen pen;
    pen.setColor(colorPrimary);

    auto rect = sceneBoundingRect();

    pen.setWidthF(0);
    painter->setPen(pen);
    painter->setBrush(colorPrimary);
    painter->drawRect(rect);

    painter->scale(1.0 / scaleX, 1.0 / scaleY);

    pen.setColor(QColor(255, 255, 255));
    painter->setPen(pen);
    auto left = rect.left();
    auto top = rect.top();
    auto width = rect.width() * scaleX;
    auto height = rect.height() * scaleY;
    auto textRect = QRectF(left, top, width, height);
    auto font = QFont("Microsoft Yahei UI");
    font.setPointSizeF(10);
    painter->setFont(font);
    painter->drawText(textRect, m_text, QTextOption(Qt::AlignVCenter));
}

void NoteGraphicsItem::setText(const QString &text) {
    m_text = text;
    update();
}

void NoteGraphicsItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    QMenu menu;
    auto action = menu.addAction("Action");
    menu.exec(event->screenPos());

    QGraphicsItem::contextMenuEvent(event);
}
