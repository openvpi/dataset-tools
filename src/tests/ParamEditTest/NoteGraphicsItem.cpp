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
    auto colorPrimary = QColor(112, 156, 255);
    auto colorPrimaryDarker = QColor(81, 135, 255);
    auto colorAccent = QColor(255, 175, 95);
    auto colorAccentDarker = QColor(255, 159, 63);
    auto scaleX = painter->transform().m11();
    auto scaleY = painter->transform().m22();
    auto penWidth = 2.0f;

    QPen pen;
    pen.setColor(isSelected() ? colorAccentDarker : colorPrimaryDarker);

    auto rect = sceneBoundingRect();
    auto left = rect.left() * scaleX + scaleX / 2 + penWidth / 2;
    auto top = rect.top() * scaleY + scaleY / 2 + penWidth / 2;
    auto width = rect.width() * scaleX - scaleX - penWidth;
    auto height = rect.height() * scaleY - scaleY - penWidth;
    auto scaledRect = QRectF(left, top, width, height);

    pen.setWidthF(penWidth);
    painter->setPen(pen);
    painter->setBrush(isSelected() ? colorAccent : colorPrimary);

    painter->scale(1.0 / scaleX, 1.0 / scaleY);
    painter->drawRoundedRect(scaledRect, 4, 4);

    pen.setColor(QColor(255, 255, 255));
    painter->setPen(pen);
    auto font = QFont("Microsoft Yahei UI");
    font.setPointSizeF(10);
    painter->setFont(font);
    int padding = 2;
    auto textRectLeft = scaledRect.left() + padding;
    auto textRectTop = scaledRect.top() + padding;
    auto textRectWidth = scaledRect.width() - 2 * padding;
    auto textRectHeight = scaledRect.height() - 2 * padding;
    auto textRect = QRectF(textRectLeft, textRectTop, textRectWidth, textRectHeight);

    auto fontMetrics = painter->fontMetrics();
    auto textHeight = fontMetrics.height();
    auto textWidth = fontMetrics.horizontalAdvance(m_text);

    if (textWidth <= textRectWidth && textHeight <= textRectHeight)
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

NoteGraphicsItem::NoteGraphicsItem(QGraphicsItem *parent) : QGraphicsRectItem(parent) {
    setFlag(QGraphicsItem::ItemIsSelectable);
}

void NoteGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
//    if (event->button() == Qt::LeftButton) {
//        if (event->modifiers() == Qt::ShiftModifier)
//            setSelected(true);
//    }

    QGraphicsItem::mousePressEvent(event);
}
