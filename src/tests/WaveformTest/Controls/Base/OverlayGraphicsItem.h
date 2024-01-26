//
// Created by fluty on 2024/1/25.
//

#ifndef OVERLAYGRAPHICSITEM_H
#define OVERLAYGRAPHICSITEM_H

#include "CommonGraphicsRectItem.h"

class OverlayGraphicsItem : public CommonGraphicsRectItem {
    Q_OBJECT

public:
    bool transparentForMouseEvents() const;
    void setTransparentForMouseEvents(bool on);
    QColor backgroundColor() const;
    void setBackgroundColor(QColor color);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    QColor m_backgroundColor = QColor(0, 0, 0, 20);
    bool m_transparentForMouseEvents = true;
};

#endif // OVERLAYGRAPHICSITEM_H
