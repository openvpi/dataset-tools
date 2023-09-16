//
// Created by fluty on 2023/9/16.
//

#include <QWheelEvent>

#include "ParamGraphicsView.h"

void ParamGraphicsView::wheelEvent(QWheelEvent *event) {
    auto deltaY = event->angleDelta().y();
    qreal scaleX = 1;
    qreal scaleY = 1;

    if (event->modifiers() == Qt::ControlModifier) {
        if (deltaY > 0)
            scaleX = 1.2;
        else if (deltaY < 0)
            scaleX = 1.0 / 1.2;
    }

    if (event->modifiers() == Qt::ShiftModifier) {
        if (deltaY > 0)
            scaleY = 1.2;
        else if (deltaY < 0)
            scaleY = 1.0 / 1.2;
    }

    scale(scaleX, scaleY);
    QGraphicsView::wheelEvent(event);
}
