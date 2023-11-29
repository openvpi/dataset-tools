//
// Created by fluty on 2023/11/14.
//
#include <QCursor>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QPainter>

#include "ClipGraphicsItem.h"
#include "TracksGraphicsScene.h"


ClipGraphicsItem::ClipGraphicsItem(QGraphicsItem *parent) : QGraphicsRectItem(parent) {
    setAcceptHoverEvents(true);
    // setFlag(QGraphicsItem::ItemIsFocusable, true); // Must enabled, or keyPressEvent would not work.
    // setAcceptTouchEvents(true);
}
ClipGraphicsItem::~ClipGraphicsItem() {
}

QString ClipGraphicsItem::name() const {
    return m_name;
}

void ClipGraphicsItem::setName(const QString &text) {
    m_name = text;
    update();
}
int ClipGraphicsItem::start() const {
    return m_start;
}

void ClipGraphicsItem::setStart(const int start) {
    m_start = start;
    updateRectAndPos();
}
int ClipGraphicsItem::length() const {
    return m_length;
}
void ClipGraphicsItem::setLength(const int length) {
    m_length = length;
    updateRectAndPos();
}
int ClipGraphicsItem::clipStart() const {
    return m_clipStart;
}
void ClipGraphicsItem::setClipStart(const int clipStart) {
    m_clipStart = clipStart;
    updateRectAndPos();
}
int ClipGraphicsItem::clipLen() const {
    return m_clipLen;
}
void ClipGraphicsItem::setClipLen(const int clipLen) {
    m_clipLen = clipLen;
    updateRectAndPos();
}

double ClipGraphicsItem::gain() const {
    return m_gain;
}
void ClipGraphicsItem::setGain(const double gain) {
    m_gain = gain;
    update();
}
double ClipGraphicsItem::scaleX() const {
    return m_scaleX;
}
double ClipGraphicsItem::scaleY() const {
    return m_scaleY;
}
void ClipGraphicsItem::setScaleX(const double scaleX) {
    m_scaleX = scaleX;
    updateRectAndPos();
}
void ClipGraphicsItem::setScaleY(const double scaleY) {
    m_scaleY = scaleY;
    updateRectAndPos();
}
int ClipGraphicsItem::trackIndex() const {
    return m_trackIndex;
}
void ClipGraphicsItem::setTrackIndex(const int index) {
    m_trackIndex = index;
    updateRectAndPos();
}
QRectF ClipGraphicsItem::previewRect() const {
    auto penWidth = 2.0f;
    auto paddingTop = 20;
    auto rect = boundingRect();
    auto left = rect.left() + penWidth;
    auto top = rect.top() + paddingTop + penWidth;
    auto width = rect.width() - penWidth * 2;
    auto height = rect.height() - paddingTop - penWidth * 2;
    auto paddedRect = QRectF(left, top, width, height);
    return paddedRect;
}
void ClipGraphicsItem::setVisibleRect(const QRectF &rect) {
    m_visibleRect = rect;
    update();
}

void ClipGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    auto colorPrimary = QColor(112, 156, 255, 220);
    auto colorPrimaryDarker = QColor(81, 135, 255, 220);
    auto colorAccent = QColor(255, 175, 95);
    auto colorAccentDarker = QColor(255, 159, 63);
    auto penWidth = 2.0f;

    QPen pen;
    pen.setColor(isSelected() ? colorAccentDarker : colorPrimaryDarker);

    auto rect = boundingRect();
    auto left = rect.left() + penWidth;
    auto top = rect.top() + penWidth;
    auto width = rect.width() - penWidth * 2;
    auto height = rect.height() - penWidth * 2;
    auto paddedRect = QRectF(left, top, width, height);

    pen.setWidthF(penWidth);
    painter->setPen(pen);
    painter->setBrush(isSelected() ? colorAccent : colorPrimary);

    painter->drawRoundedRect(paddedRect, 4, 4);

    auto font = QFont("Microsoft Yahei UI");
    font.setPointSizeF(10);
    painter->setFont(font);
    int textPadding = 2;
    auto rectLeft = mapToScene(rect.topLeft()).x();
    auto rectRight = mapToScene(rect.bottomRight()).x();
    auto textRectLeft = m_visibleRect.left() < rectLeft ? paddedRect.left() + textPadding
                                                        : m_visibleRect.left() - rectLeft + textPadding + penWidth;
    auto textRectTop = paddedRect.top() + textPadding;
    auto textRectWidth = m_visibleRect.right() < rectRight ? m_visibleRect.width() - 2 * textPadding - penWidth
                                                           : rectRight - m_visibleRect.left() - 2 * textPadding;
    auto textRectHeight = paddedRect.height() - 2 * textPadding;
    auto textRect = QRectF(textRectLeft, textRectTop, textRectWidth, textRectHeight);

    auto fontMetrics = painter->fontMetrics();
    auto textHeight = fontMetrics.height();
    auto controlStr = QString("%1 %2dB %3 ").arg(m_name).arg(QString::number(m_gain)).arg(m_mute ? "M" : "");
    auto timeStr = QString("s: %1 l: %2 cs: %3 cl: %4").arg(m_start).arg(m_length).arg(m_clipStart).arg(m_clipLen);
    auto text = controlStr + timeStr;
    auto textWidth = fontMetrics.horizontalAdvance(text);

    pen.setColor(QColor(255, 255, 255));
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    if (textWidth <= textRectWidth && textHeight <= textRectHeight)
        painter->drawText(textRect, text);
}

void ClipGraphicsItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    QGraphicsItem::contextMenuEvent(event);
}

void ClipGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    m_mouseDownPos = event->scenePos();
    m_mouseDownStart = start();
    m_mouseDownClipStart = clipStart();
    m_mouseDownLength = length();
    m_mouseDownClipLen = clipLen();
    event->accept(); // Must accept event, or mouseMoveEvent would not work.
}
void ClipGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    auto curPos = event->scenePos();
    if (event->modifiers() == Qt::AltModifier)
        m_tempQuantizeOff = true;
    else
        m_tempQuantizeOff = false;

    auto dx = (curPos.x() - m_mouseDownPos.x()) / m_scaleX / pixelPerQuarterNote * 480;

    // snap tick to grid
    auto roundPos = [](int i, int step) {
        int times = i / step;
        int mod = i % step;
        if (mod > step / 2)
            return step * (times + 1);
        else
            return step * times;
    };

    int start;
    int clipStart;
    int clipLen;
    int deltaClipStart;
    int delta = qRound(dx);
    int quantize = m_tempQuantizeOff ? 1 : m_quantize;
    switch (m_mouseMoveBehavior) {
        case Move:
            start = roundPos(m_mouseDownStart + delta, quantize);
            setStart(start);
            break;
        case ResizeLeft:
            clipStart = roundPos(m_mouseDownClipStart + delta, quantize);
            deltaClipStart = clipStart - m_mouseDownClipStart;
            clipLen = m_mouseDownClipLen - deltaClipStart;
            if (clipStart < 0) {
                setClipStart(0);
                setClipLen(m_mouseDownClipStart + m_mouseDownClipLen);
            } else if (clipStart <= m_mouseDownClipStart + m_mouseDownClipLen) {
                setClipStart(clipStart);
                setClipLen(clipLen);
            } else {
                setClipStart(m_mouseDownClipStart + m_mouseDownClipLen);
                setClipLen(0);
            }
            break;
        case ResizeRight:
            clipLen = roundPos(m_mouseDownClipLen + delta, quantize);
            // TODO: check if length can be longer than before ( singing clip )
            if (clipLen >= m_length)
                setClipLen(m_length);
            else if (clipLen >= 0) {
                setClipLen(clipLen);
            }
            break;
    }
    QGraphicsRectItem::mouseMoveEvent(event);
}
void ClipGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    QGraphicsRectItem::hoverEnterEvent(event);
}
void ClipGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    QGraphicsRectItem::hoverLeaveEvent(event);
}
void ClipGraphicsItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
    const auto rx = event->pos().rx();
    if (rx >= 0 && rx <= m_resizeTolerance) {
        setCursor(Qt::SizeHorCursor);
        m_mouseMoveBehavior = ResizeLeft;
    } else if (rx >= rect().width() - m_resizeTolerance && rx <= rect().width()) {
        setCursor(Qt::SizeHorCursor);
        m_mouseMoveBehavior = ResizeRight;
    } else {
        setCursor(Qt::ArrowCursor);
        m_mouseMoveBehavior = Move;
    }

    QGraphicsRectItem::hoverMoveEvent(event);
}
// void ClipGraphicsItem::keyPressEvent(QKeyEvent *event) {
//     if (event->key() == Qt::Key_Alt)
//         m_tempQuantizeOff = true;
//     qDebug() << "alt down";
//     QGraphicsRectItem::keyPressEvent(event);
// }
// void ClipGraphicsItem::keyReleaseEvent(QKeyEvent *event) {
//     if (event->key() == Qt::Key_Alt)
//         m_tempQuantizeOff = false;
//     QGraphicsRectItem::keyReleaseEvent(event);
// }

void ClipGraphicsItem::updateRectAndPos() {
    auto x = (m_start + m_clipStart) * m_scaleX * pixelPerQuarterNote / 480;
    auto y = m_trackIndex * trackHeight * m_scaleY;
    auto w = m_clipLen * m_scaleX * pixelPerQuarterNote / 480;
    auto h = trackHeight * m_scaleY;
    setPos(x, y);
    setRect(QRectF(0, 0, w, h));
    update();
}
