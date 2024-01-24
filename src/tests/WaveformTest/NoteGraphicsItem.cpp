//
// Created by fluty on 2024/1/23.
//

#include <QInputDialog>
#include <QMenu>
#include <QPainter>
#include <QGraphicsSceneContextMenuEvent>

#include "NoteGraphicsItem.h"

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
double NoteGraphicsItem::scaleX() const {
    return m_scaleX;
}
void NoteGraphicsItem::setScaleX(double scaleX) {
    m_scaleX = scaleX;
    updateRectAndPos();
}
double NoteGraphicsItem::scaleY() const {
    return m_scaleY;
}
void NoteGraphicsItem::setScaleY(double scaleY) {
    m_scaleY = scaleY;
    updateRectAndPos();
}
void NoteGraphicsItem::setVisibleRect(const QRectF &rect) {
    m_visibleRect = rect;
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
    auto left = rect.left() + penWidth;
    auto top = rect.top() + penWidth;
    auto width = rect.width() - penWidth * 2;
    auto height = rect.height() - penWidth * 2;
    auto paddedRect = QRectF(left, top, width, height);

    pen.setWidthF(penWidth);
    painter->setPen(pen);
    painter->setBrush(colorPrimary);

    if (paddedRect.width() < 8 || paddedRect.height() < 8) { // draw rect without border
        painter->setRenderHint(QPainter::Antialiasing, false);
        painter->setPen(Qt::NoPen);
        painter->setBrush(borderColor);
        auto l = boundingRect().left() + 1;
        auto t = boundingRect().top() + 1;
        auto w = boundingRect().width() < 4 ? 2 : boundingRect().width() - 2;
        auto h = boundingRect().height() < 4 ? 2 : boundingRect().height() - 2;
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
        painter->drawRoundedRect(paddedRect, 1, 1);
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
    auto textWidth = fontMetrics.horizontalAdvance(m_lyric);

    if (textWidth <= textRectWidth && textHeight <= textRectHeight) {
        // draw lryic
        painter->drawText(textRect, m_lyric, QTextOption(Qt::AlignVCenter));
        // draw pronunciation
        pen.setColor(QColor(160, 160, 160));
        painter->setPen(pen);
        painter->drawText(QPointF(textRectLeft, textRectTop - 10), m_pronunciation);
    }
}
void NoteGraphicsItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    QMenu menu;
    auto editLyricAction = menu.addAction("Edit Lyric");
    connect(editLyricAction, &QAction::triggered, [&]() {
        auto ok = false;
        auto result = QInputDialog::getText(graphicsView, "Edit Lyric", "Input lyric:", QLineEdit::Normal, m_lyric, &ok);
        if (ok && !result.isEmpty())
            setLyric(result);
    });

    auto editPronunciationAction = menu.addAction("Edit Pronunciation");
    connect(editPronunciationAction, &QAction::triggered, [&]() {
        auto ok = false;
        auto result = QInputDialog::getText(graphicsView, "Edit Pronunciation", "Input pronunciation:", QLineEdit::Normal, m_pronunciation, &ok);
        if (ok && !result.isEmpty())
            setPronunciation(result);
    });
    menu.exec(event->screenPos());

    QGraphicsItem::contextMenuEvent(event);
}
void NoteGraphicsItem::updateRectAndPos() {
    const auto x = m_start * m_scaleX * pixelPerQuarterNote / 480;
    const auto y = -(m_keyIndex - 127) * noteHeight * m_scaleY;
    const auto w = m_length * m_scaleX * pixelPerQuarterNote / 480;
    const auto h = noteHeight * m_scaleY;
    setPos(x, y);
    setRect(QRectF(0, 0, w, h));
    update();
}
void NoteGraphicsItem::initUi() {
    setAcceptHoverEvents(true);
    setFlag(ItemIsSelectable);
}