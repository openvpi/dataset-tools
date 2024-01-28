//
// Created by fluty on 2024/1/23.
//

#include <QDebug>
#include <QGraphicsSceneContextMenuEvent>
#include <QInputDialog>
#include <QMenu>
#include <QPainter>
#include <QTextOption>

#include "NoteGraphicsItem.h"
#include "PianoRollGlobal.h"

using namespace PianoRollGlobal;

NoteGraphicsItem::NoteGraphicsItem(int itemId, QGraphicsItem *parent) {
    m_itemId = itemId;
    initUi();
}
NoteGraphicsItem::NoteGraphicsItem(int itemId, int start, int length, int keyIndex, const QString &lyric,
                                   const QString &pronunciation, QGraphicsItem *parent) {
    m_itemId = itemId;
    m_start = start;
    m_length = length;
    m_keyIndex = keyIndex;
    m_lyric = lyric;
    m_pronunciation = pronunciation;
    initUi();
}
int NoteGraphicsItem::start() const {
    return m_start;
}
void NoteGraphicsItem::setStart(int start) {
    m_start = start;
    updateRectAndPos();
}
int NoteGraphicsItem::length() const {
    return m_length;
}
void NoteGraphicsItem::setLength(int length) {
    m_length = length;
    updateRectAndPos();
}
int NoteGraphicsItem::keyIndex() const {
    return m_keyIndex;
}
void NoteGraphicsItem::setKeyIndex(int keyIndex) {
    m_keyIndex = keyIndex;
    updateRectAndPos();
}
QString NoteGraphicsItem::lyric() const {
    return m_lyric;
}
void NoteGraphicsItem::setLyric(const QString &lyric) {
    m_lyric = lyric;
    update();
}
QString NoteGraphicsItem::pronunciation() const {
    return m_pronunciation;
}
void NoteGraphicsItem::setPronunciation(const QString &pronunciation) {
    m_pronunciation = pronunciation;
    update();
}
void NoteGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    const auto colorPrimary = QColor(155, 186, 255);
    const auto colorPrimaryDarker = QColor(112, 156, 255);
    const auto colorAccent = QColor(255, 175, 95);
    const auto colorAccentDarker = QColor(255, 159, 63);
    const auto colorForeground = QColor(0, 0, 0);
    const auto penWidth = 2.0f;
    const auto radius = 4.0;
    const auto radiusAdjustThreshold = 12;

    auto borderColor = isSelected() ? QColor(255, 255, 255) : colorPrimaryDarker;
    QPen pen;
    pen.setColor(borderColor);

    auto rect = boundingRect();
    auto noteBoundingRect = QRectF(rect.left(), rect.top(), rect.width(), rect.height() - m_pronunciationTextHeight);
    auto left = noteBoundingRect.left() + penWidth;
    auto top = noteBoundingRect.top() + penWidth;
    auto width = noteBoundingRect.width() - penWidth * 2;
    auto height = noteBoundingRect.height() - penWidth * 2;
    auto paddedRect = QRectF(left, top, width, height);

    pen.setWidthF(penWidth);
    painter->setPen(pen);
    painter->setBrush(colorPrimary);

    if (paddedRect.width() < 8 || paddedRect.height() < 8) { // draw rect without border
        painter->setRenderHint(QPainter::Antialiasing, false);
        painter->setPen(Qt::NoPen);
        painter->setBrush(isSelected() ? QColor(255, 255, 255) : colorPrimary);
        auto l = noteBoundingRect.left() + penWidth / 2;
        auto t = noteBoundingRect.top() + penWidth / 2;
        auto w = noteBoundingRect.width() - penWidth < 2 ? 2 : noteBoundingRect.width() - penWidth;
        auto h = noteBoundingRect.height() - penWidth < 2 ? 2 : noteBoundingRect.height() - penWidth;
        painter->drawRect(QRectF(l, t, w, h));
    } else {
        // auto straightX = paddedRect.width() - radius * 2;
        // auto straightY = paddedRect.height() - radius * 2;
        // auto xRadius = radius;
        // auto yRadius = radius;
        // if (straightX < radiusAdjustThreshold)
        //     xRadius = radius * (straightX - radius) / (radiusAdjustThreshold - radius);
        // if (straightY < radiusAdjustThreshold)
        //     yRadius = radius * (straightY - radius) / (radiusAdjustThreshold - radius);
        // painter->drawRoundedRect(paddedRect, xRadius, yRadius);
        painter->drawRoundedRect(paddedRect, 2, 2);
    }

    pen.setColor(colorForeground);
    painter->setPen(pen);
    auto font = QFont("Microsoft Yahei UI");
    font.setPointSizeF(10);
    painter->setFont(font);
    int padding = 2;
    // TODO: keep lyric on screen
    auto textRectLeft = paddedRect.left() + padding;
    // auto textRectTop = paddedRect.top() + padding;
    auto textRectTop = paddedRect.top();
    auto textRectWidth = paddedRect.width() - 2 * padding;
    // auto textRectHeight = paddedRect.height() - 2 * padding;
    auto textRectHeight = paddedRect.height();
    auto textRect = QRectF(textRectLeft, textRectTop, textRectWidth, textRectHeight);

    auto fontMetrics = painter->fontMetrics();
    auto textHeight = fontMetrics.height();
    auto text = m_lyric;
    auto textWidth = fontMetrics.horizontalAdvance(text);
    QTextOption textOption(Qt::AlignVCenter);
    textOption.setWrapMode(QTextOption::NoWrap);

    if (textWidth < textRectWidth && textHeight < textRectHeight) {
        // draw lryic
        painter->drawText(textRect, text, textOption);
        // draw pronunciation
        pen.setColor(QColor(200, 200, 200));
        painter->setPen(pen);
        painter->drawText(QPointF(textRectLeft, boundingRect().bottom() - 6), m_pronunciation);
    }
}
void NoteGraphicsItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    // TODO: notify controller
    // QMenu menu;
    // auto editLyricAction = menu.addAction("Edit Lyric");
    // connect(editLyricAction, &QAction::triggered, [&]() {
    //     auto ok = false;
    //     auto result =
    //         QInputDialog::getText(graphicsView, "Edit Lyric", "Input lyric:", QLineEdit::Normal, m_lyric, &ok);
    //     if (ok && !result.isEmpty())
    //         setLyric(result);
    // });
    //
    // auto editPronunciationAction = menu.addAction("Edit Pronunciation");
    // connect(editPronunciationAction, &QAction::triggered, [&]() {
    //     auto ok = false;
    //     auto result = QInputDialog::getText(graphicsView, "Edit Pronunciation",
    //                                         "Input pronunciation:", QLineEdit::Normal, m_pronunciation, &ok);
    //     if (ok && !result.isEmpty())
    //         setPronunciation(result);
    // });
    // menu.exec(event->screenPos());

    QGraphicsItem::contextMenuEvent(event);
}
void NoteGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        m_mouseMoveBehavior = None;
        setCursor(Qt::ArrowCursor);
    }

    m_mouseDownPos = event->scenePos();
    m_mouseDownStart = start();
    m_mouseDownLength = length();
    m_mouseDownKeyIndex = keyIndex();
    event->accept();
}
void NoteGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    auto curPos = event->scenePos();
    if (event->modifiers() == Qt::AltModifier)
        m_tempQuantizeOff = true;
    else
        m_tempQuantizeOff = false;

    auto dx = (curPos.x() - m_mouseDownPos.x()) / scaleX() / pixelsPerQuarterNote * 480;
    auto dy = (curPos.y() - m_mouseDownPos.y()) / scaleY() / noteHeight;

    // snap tick to grid
    auto roundPos = [](int i, int step) {
        int times = i / step;
        int mod = i % step;
        if (mod > step / 2)
            return step * (times + 1);
        return step * times;
    };

    int start;
    int deltaStart;
    int length;
    int right;
    int keyIndex;
    int deltaX = qRound(dx);
    int deltaY = qRound(dy);
    int quantize = m_tempQuantizeOff ? 1 : m_quantize;
    switch (m_mouseMoveBehavior) {
        case Move:
            start = roundPos(m_mouseDownStart + deltaX, quantize);
            setStart(start);

            keyIndex = m_mouseDownKeyIndex - deltaY;
            if (keyIndex > 127)
                keyIndex = 127;
            else if (keyIndex < 0)
                keyIndex = 0;

            setKeyIndex(keyIndex);
            break;

        case ResizeLeft:
            start = roundPos(m_mouseDownStart + deltaX, quantize);
            deltaStart = start - m_mouseDownStart;
            length = m_mouseDownLength - deltaStart;
            if (length > 0) {
                setStart(start);
                setLength(length);
            }
            break;

        case ResizeRight:
            right = roundPos(m_mouseDownStart + m_mouseDownLength + deltaX, quantize);
            length = right - m_mouseDownStart;
            if (length > 0)
                setLength(length);
            break;

        case None:
            QGraphicsRectItem::mouseMoveEvent(event);
    }
}
void NoteGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event) {
    QGraphicsRectItem::hoverEnterEvent(event);
}
void NoteGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event) {
    QGraphicsRectItem::hoverLeaveEvent(event);
}
void NoteGraphicsItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event) {
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
void NoteGraphicsItem::updateRectAndPos() {
    const auto x = m_start * scaleX() * pixelsPerQuarterNote / 480;
    const auto y = -(m_keyIndex - 127) * noteHeight * scaleY();
    const auto w = m_length * scaleX() * pixelsPerQuarterNote / 480;
    const auto h = noteHeight * scaleY() + m_pronunciationTextHeight;
    setPos(x, y);
    setRect(QRectF(0, 0, w, h));
    update();
}
void NoteGraphicsItem::initUi() {
    setAcceptHoverEvents(true);
    setFlag(ItemIsSelectable);
}