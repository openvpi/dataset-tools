//
// Created by fluty on 2024/1/26.
//

#include "CommonGraphicsRectItem.h"

CommonGraphicsRectItem::CommonGraphicsRectItem(QGraphicsItem *parent) : QGraphicsRectItem(parent) {
}
double CommonGraphicsRectItem::scaleX() const {
    return m_scaleX;
}
void CommonGraphicsRectItem::setScaleX(double scaleX) {
    m_scaleX = scaleX;
    updateRectAndPos();
}
double CommonGraphicsRectItem::scaleY() const {
    return m_scaleY;
}
void CommonGraphicsRectItem::setScaleY(double scaleY) {
    m_scaleY = scaleY;
    updateRectAndPos();
}
QRectF CommonGraphicsRectItem::visibleRect() const {
    return m_visibleRect;
}
void CommonGraphicsRectItem::setVisibleRect(const QRectF &rect) {
    m_visibleRect = rect;
    updateRectAndPos();
}