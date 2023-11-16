//
// Created by fluty on 2023/11/16.
//

#include "AudioClipGraphicsItem.h"


#include <QPainter>
AudioClipGraphicsItem::AudioClipGraphicsItem(QGraphicsItem *parent) {
}
AudioClipGraphicsItem::~AudioClipGraphicsItem() {
}
void AudioClipGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    // Draw frame
    ClipGraphicsItem::paint(painter, option, widget);

    // Draw waveform in previewRect() when file loaded
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(255, 255, 255, 80));
    painter->drawRect(previewRect());
    painter->setPen(Qt::white);
    painter->drawText(previewRect(), "Waveform", QTextOption(Qt::AlignCenter));
}