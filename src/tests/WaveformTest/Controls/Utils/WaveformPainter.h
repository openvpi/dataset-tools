//
// Created by fluty on 2024/1/26.
//

#ifndef WAVEFORMPAINTER_H
#define WAVEFORMPAINTER_H

#include <QPainter>
#include <QRectF>

class WaveformPainter {
public:
    void drawWaveform(QPainter *painter, const QRectF &rect);
};

#endif //WAVEFORMPAINTER_H
