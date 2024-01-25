//
// Created by fluty on 2024/1/25.
//

#include <QGraphicsSceneMouseEvent>
#include <QPainter>

#include "OverlayGraphicsItem.h"

bool OverlayGraphicsItem::transparentForMouseEvents() const {
    return m_transparentForMouseEvents;
}
void OverlayGraphicsItem::setTransparentForMouseEvents(bool on) {
    m_transparentForMouseEvents = on;
    setAcceptHoverEvents(!m_transparentForMouseEvents);
    update();
}
QColor OverlayGraphicsItem::backgroundColor() const {
    return m_backgroundColor;
}
void OverlayGraphicsItem::setBackgroundColor(QColor color) {
    m_backgroundColor = color;
    update();
}
void OverlayGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    painter->setBrush(m_backgroundColor);
    painter->drawRect(boundingRect());
}
void OverlayGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (!m_transparentForMouseEvents)
        event->accept();
    else
        QGraphicsRectItem::mousePressEvent(event);
}