//
// Created by fluty on 2023/11/14.
//
#include <QCursor>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QPainter>

#include "AbstractClipGraphicsItem.h"
#include "TracksEditorGlobal.h"
#include "TracksGraphicsScene.h"

using namespace TracksEditorGlobal;

AbstractClipGraphicsItem::AbstractClipGraphicsItem(int itemId, QGraphicsItem *parent) : CommonGraphicsRectItem(parent) {
    m_itemId = itemId;
    setAcceptHoverEvents(true);
    setFlag(ItemIsSelectable);
    // setFlag(QGraphicsItem::ItemIsFocusable, true); // Must enabled, or keyPressEvent would not work.
    // setAcceptTouchEvents(true);
}
int AbstractClipGraphicsItem::itemId() {
    return m_itemId;
}

QString AbstractClipGraphicsItem::name() const {
    return m_name;
}

void AbstractClipGraphicsItem::setName(const QString &text) {
    m_name = text;
    emit nameChanged(text);
    update();
}
int AbstractClipGraphicsItem::start() const {
    return m_start;
}

void AbstractClipGraphicsItem::setStart(const int start) {
    m_start = start;
    // qDebug() << "ClipGraphicsItem::setStart" << start;
    emit startChanged(m_itemId, start);
    updateRectAndPos();
}
int AbstractClipGraphicsItem::length() const {
    return m_length;
}
void AbstractClipGraphicsItem::setLength(const int length) {
    m_length = length;
    emit lengthChanged(length);
    updateRectAndPos();
}
int AbstractClipGraphicsItem::clipStart() const {
    return m_clipStart;
}
void AbstractClipGraphicsItem::setClipStart(const int clipStart) {
    m_clipStart = clipStart;
    emit clipStartChanged(clipStart);
    updateRectAndPos();
}
int AbstractClipGraphicsItem::clipLen() const {
    return m_clipLen;
}
void AbstractClipGraphicsItem::setClipLen(const int clipLen) {
    m_clipLen = clipLen;
    emit clipLenChanged(clipLen);
    updateRectAndPos();
}

double AbstractClipGraphicsItem::gain() const {
    return m_gain;
}
void AbstractClipGraphicsItem::setGain(const double gain) {
    m_gain = gain;
    update();
}
bool AbstractClipGraphicsItem::mute() const {
    return m_mute;
}
void AbstractClipGraphicsItem::setMute(bool mute) {
    m_mute = mute;
    update();
}
int AbstractClipGraphicsItem::trackIndex() const {
    return m_trackIndex;
}
void AbstractClipGraphicsItem::setTrackIndex(const int index) {
    m_trackIndex = index;
    updateRectAndPos();
}
QRectF AbstractClipGraphicsItem::previewRect() const {
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

void AbstractClipGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    const auto colorPrimary = QColor(155, 186, 255);
    const auto colorPrimaryDarker = QColor(112, 156, 255);
    const auto colorAccent = QColor(255, 175, 95);
    const auto colorAccentDarker = QColor(255, 159, 63);
    const auto colorForeground = QColor(0, 0, 0);
    auto penWidth = 2.0f;

    QPen pen;
    pen.setColor(isSelected() ? QColor(255, 255, 255) : colorPrimaryDarker);

    auto rect = boundingRect();
    auto left = rect.left() + penWidth;
    auto top = rect.top() + penWidth;
    auto width = rect.width() - penWidth * 2;
    auto height = rect.height() - penWidth * 2;
    auto paddedRect = QRectF(left, top, width, height);

    pen.setWidthF(penWidth);
    painter->setPen(pen);
    painter->setBrush(colorPrimary);

    painter->drawRoundedRect(paddedRect, 4, 4);

    auto font = QFont("Microsoft Yahei UI");
    font.setPointSizeF(10);
    painter->setFont(font);
    int textPadding = 2;
    auto rectLeft = mapToScene(rect.topLeft()).x();
    auto rectRight = mapToScene(rect.bottomRight()).x();
    auto textRectLeft = visibleRect().left() < rectLeft ? paddedRect.left() + textPadding
                                                        : visibleRect().left() - rectLeft + textPadding + penWidth;
    auto textRectTop = paddedRect.top() + textPadding;
    auto textRectWidth = rectRight - visibleRect().left() - 2 * (textPadding + penWidth);
    auto textRectHeight = paddedRect.height() - 2 * textPadding;
    auto textRect = QRectF(textRectLeft, textRectTop, textRectWidth, textRectHeight);

    auto fontMetrics = painter->fontMetrics();
    auto textHeight = fontMetrics.height();
    auto controlStr = QString("%1 %2dB %3 ").arg(m_name).arg(QString::number(m_gain)).arg(m_mute ? "M" : "");
    auto timeStr = QString("s: %1 l: %2 cs: %3 cl: %4 sx: %5 sy: %6")
                       .arg(m_start)
                       .arg(m_length)
                       .arg(m_clipStart)
                       .arg(m_clipLen)
                       .arg(scaleX())
                       .arg(scaleY());
    auto text = clipTypeName() + controlStr + timeStr;
    auto textWidth = fontMetrics.horizontalAdvance(text);

    pen.setColor(colorForeground);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    // painter->drawRect(textRect);
    if (textWidth <= textRectWidth && textHeight <= textRectHeight) {
        if (previewRect().height() < 32)
            painter->drawText(textRect, text, QTextOption(Qt::AlignVCenter));
        else
            painter->drawText(textRect, text);
    }

    auto previewRectHeight = previewRect().height();
    if (previewRectHeight >= 32) {
        auto colorAlpha = previewRectHeight <= 48 ? static_cast<int>(255 * (previewRectHeight - 32) / (48 - 32)) : 255;
        drawPreviewArea(painter, previewRect(), colorAlpha);
    }
}

void AbstractClipGraphicsItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    QMenu menu;
    auto actionRename = menu.addAction("Rename");
    QObject::connect(actionRename, &QAction::triggered, [this]() { qDebug() << "rename triggered"; });
    menu.exec(event->screenPos());

    QGraphicsItem::contextMenuEvent(event);
}

void AbstractClipGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        m_mouseMoveBehavior = None;
        setCursor(Qt::ArrowCursor);
    }

    // setSelected(true);
    m_mouseDownPos = event->scenePos();
    m_mouseDownStart = start();
    m_mouseDownClipStart = clipStart();
    m_mouseDownLength = length();
    m_mouseDownClipLen = clipLen();
    event->accept(); // Must accept event, or mouseMoveEvent would not work.
}
void AbstractClipGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    auto curPos = event->scenePos();
    if (event->modifiers() == Qt::AltModifier)
        m_tempQuantizeOff = true;
    else
        m_tempQuantizeOff = false;

    auto dx = (curPos.x() - m_mouseDownPos.x()) / scaleX() / pixelsPerQuarterNote * 480;

    // snap tick to grid
    auto roundPos = [](int i, int step) {
        int times = i / step;
        int mod = i % step;
        if (mod > step / 2)
            return step * (times + 1);
        return step * times;
    };

    int start;
    int clipStart;
    int left;
    int clipLen;
    int right;
    int deltaClipStart;
    int delta = qRound(dx);
    int quantize = m_tempQuantizeOff ? 1 : m_quantize;
    switch (m_mouseMoveBehavior) {
        case Move:
            left = roundPos(m_mouseDownStart + m_mouseDownClipStart + delta, quantize);
            start = left - m_mouseDownClipStart;
            setStart(start);
            break;
        case ResizeLeft:
            left = roundPos(m_mouseDownStart + m_mouseDownClipStart + delta, quantize);
            start = m_mouseDownStart;
            clipStart = left - start;
            clipLen = m_mouseDownStart + m_mouseDownClipStart + m_mouseDownClipLen - left;
            if (clipLen <= 0)
                break;

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
            right = roundPos(m_mouseDownStart + m_mouseDownClipStart + m_mouseDownClipLen + delta, quantize);
            clipLen = right - (m_mouseDownStart + m_mouseDownClipStart);
            if (clipLen <= 0)
                break;

            if (!m_canResizeLength) {
                if (clipLen >= m_length)
                    setClipLen(m_length);
                else
                    setClipLen(clipLen);
            } else {
                setLength(m_clipStart + clipLen);
                setClipLen(clipLen);
            }
            break;
        case None:
            break;
    }
    QGraphicsRectItem::mouseMoveEvent(event);
}
void AbstractClipGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    QGraphicsRectItem::hoverEnterEvent(event);
}
void AbstractClipGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    QGraphicsRectItem::hoverLeaveEvent(event);
}
void AbstractClipGraphicsItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
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

void AbstractClipGraphicsItem::updateRectAndPos() {
    auto x = (m_start + m_clipStart) * scaleX() * pixelsPerQuarterNote / 480;
    auto y = m_trackIndex * trackHeight * scaleY();
    auto w = m_clipLen * scaleX() * pixelsPerQuarterNote / 480;
    auto h = trackHeight * scaleY();
    setPos(x, y);
    setRect(QRectF(0, 0, w, h));
    update();
}
void AbstractClipGraphicsItem::setCanResizeLength(bool on) {
    m_canResizeLength = on;
}
double AbstractClipGraphicsItem::tickToSceneX(const double tick) const {
    return tick * scaleX() * pixelsPerQuarterNote / 480;
}
double AbstractClipGraphicsItem::sceneXToItemX(const double x) const {
    return mapFromScene(QPointF(x, 0)).x();
}