//
// Created by fluty on 2024/1/21.
//

#include <QDebug>
#include <QPainter>

#include "TimeGridGraphicsItem.h"

TimeGridGraphicsItem::TimeGridGraphicsItem(QGraphicsItem *parent) {
}
double TimeGridGraphicsItem::scaleX() const {
    return m_scaleX;
}
void TimeGridGraphicsItem::setScaleX(double scaleX) {
    m_scaleX = scaleX;
    updateRectAndPos();
}
double TimeGridGraphicsItem::scaleY() const {
    return m_scaleY;
}
void TimeGridGraphicsItem::setScaleY(double scaleY) {
    m_scaleY = scaleY;
    updateRectAndPos();
}

void TimeGridGraphicsItem::onVisibleRectChanged(const QRectF &rect) {
    m_visibleRect = rect;
    updateRectAndPos();
}

void TimeGridGraphicsItem::onTimeSignatureChanged(int numerator, int denominator) {
    m_numerator = numerator;
    m_denominator = denominator;
    update();
}
void TimeGridGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    auto backgroundColor = QColor(42, 43, 44);
    auto barLineColor = QColor(92, 96, 100);
    auto beatLineColor = QColor(72, 75, 78);
    auto commonLineColor = QColor(57, 59, 61);
    auto barTextColor = QColor(200, 200, 200);
    auto beatTextColor = QColor(160, 160, 160);
    auto penWidth = 1;

    QPen pen;
    pen.setWidthF(penWidth);

    painter->setBrush(backgroundColor);
    painter->drawRect(boundingRect());
    painter->setBrush(Qt::NoBrush);

    pen.setColor(commonLineColor);
    painter->setPen(pen);
    painter->setRenderHint(QPainter::Antialiasing);

    auto sceneXToTick = [&](const double pos) { return 480 * pos / m_scaleX / pixelPerQuarterNote; };

    auto tickToSceneX = [&](const double tick) { return tick * m_scaleX * pixelPerQuarterNote / 480; };

    auto sceneXToItemX = [&](const double x) { return mapFromScene(QPointF(x, 0)).x(); };

    auto startTick = sceneXToTick(m_visibleRect.left());
    auto endTick = sceneXToTick(m_visibleRect.right());
    auto prevLineTick = static_cast<int>(startTick / 240) * 240; // 1/8 quantize
    // qDebug() << startTick << endTick << prevLineTick;
    bool canDrawEighthLine = 240 * m_scaleX * pixelPerQuarterNote / 480 >= m_minimumSpacing;
    bool canDrawQuarterLine = m_scaleX * pixelPerQuarterNote >= m_minimumSpacing;
    int barTicks = 1920 * m_numerator / m_denominator;
    int beatTicks = 1920 / m_denominator;
    // TODO: hide low level grid lines when distance < min spaceing
    for (int i = prevLineTick; i <= endTick; i += 240) {
        auto bar = i / barTicks + 1;
        auto beat = (i % barTicks) / beatTicks + 1;
        auto x = sceneXToItemX(tickToSceneX(i));
        if (i % barTicks == 0) { // bar
            pen.setColor(barTextColor);
            painter->setPen(pen);
            painter->drawText(QPointF(x, 10), QString::number(bar));
            pen.setColor(barLineColor);
            painter->setPen(pen);
            painter->drawLine(QLineF(x, 0, x, m_visibleRect.height()));
        } else if (i % beatTicks == 0 && canDrawQuarterLine) {
            pen.setColor(beatTextColor);
            painter->setPen(pen);
            painter->drawText(QPointF(x, 10), QString::number(bar) + "." + QString::number(beat));
            pen.setColor(beatLineColor);
            painter->setPen(pen);
            painter->drawLine(QLineF(x, 0, x, m_visibleRect.height()));
        } else if (canDrawEighthLine) {
            pen.setColor(commonLineColor);
            painter->setPen(pen);
            painter->drawLine(QLineF(x, 0, x, m_visibleRect.height()));
        }
    }
}

void TimeGridGraphicsItem::updateRectAndPos() {
    auto pos = m_visibleRect.topLeft();
    setPos(pos);
    setRect(QRectF(0, 0, m_visibleRect.width(), m_visibleRect.height()));
    update();
}