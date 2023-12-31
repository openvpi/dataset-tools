//
// Created by fluty on 2023/11/16.
//

#include <QDebug>
#include <QFileInfo>
#include <QPainter>
#include <QThread>

#include "AudioClipBackgroundWorker.h"
#include "AudioClipGraphicsItem.h"

AudioClipGraphicsItem::AudioClipGraphicsItem(int itemId, QGraphicsItem *parent) : ClipGraphicsItem(itemId, parent) {
}
AudioClipGraphicsItem::~AudioClipGraphicsItem() {
}
void AudioClipGraphicsItem::openFile(const QString &path) {
    m_path = path;
    m_loading = true;
    setName(QFileInfo(m_path).fileName());
    if (length() == 0)
        setLength(3840);
    if (clipLen() == 0)
        setClipLen(3840);

    m_worker = new AudioClipBackgroundWorker(path);
    connect(m_worker, &AudioClipBackgroundWorker::finished, this, &AudioClipGraphicsItem::onLoadComplete);
    // m_threadPool->start(m_worker);
    auto thread = new QThread;
    m_worker->moveToThread(thread);
    connect(thread, &QThread::started, m_worker, &AudioClipBackgroundWorker::run);
    thread->start();
}
QString AudioClipGraphicsItem::path() {
    return m_path;
}
void AudioClipGraphicsItem::onLoadComplete(bool success, QString errorMessage) {
    if (!success) {
        qDebug() << "open file error" << errorMessage;
        return;
    }

    m_peakCache.swap(m_worker->peakCache);
    delete m_worker;

    setClipStart(0);
    setLength(m_peakCache.count() / chunksPerTick);
    setClipLen(m_peakCache.count() / chunksPerTick);
    m_scale = 1.0;
    m_loading = false;

    update();
}
void AudioClipGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    // Draw frame
    ClipGraphicsItem::paint(painter, option, widget);

    painter->setRenderHint(QPainter::Antialiasing, false);

    // TODO: Draw waveform in previewRect() when file loaded

    auto rectLeft = previewRect().left();
    auto rectTop = previewRect().top();
    auto rectWidth = previewRect().width();
    auto rectHeight = previewRect().height();
    auto halfRectHeight = rectHeight / 2;
    const auto gradientRange = 48;

    if (rectHeight < 32 || rectWidth < 16)
        return;
    auto widthHeightMin = rectWidth < rectHeight ? rectWidth : rectHeight;
    auto colorAlpha = widthHeightMin <= gradientRange ? 255 * widthHeightMin / gradientRange : 255;
    auto peakColor = QColor(255, 255, 255, colorAlpha);

    QPen pen;

    if (m_loading) {
        // painter->setPen(Qt::NoPen);
        // painter->setBrush(QColor(255, 255, 255, 80));
        // painter->drawRect(previewRect());

        pen.setColor(peakColor);
        painter->setPen(pen);
        painter->drawText(previewRect(), "Loading...", QTextOption(Qt::AlignCenter));
    }

    pen.setColor(peakColor);
    pen.setWidth(1);
    painter->setPen(pen);

    auto drawPeakGraphic = [&]() {
        if (m_peakCache.count() == 0)
            return;

        // auto start = m_renderStart;
        // auto end = m_renderEnd;
        auto start = clipStart() * chunksPerTick;
        auto end = (clipStart() + clipLen()) * chunksPerTick;
        auto drawPeak = [&](int x, short min, short max) {
            auto yMin = -min * halfRectHeight / 32767 + halfRectHeight + rectTop;
            auto yMax = -max * halfRectHeight / 32767 + halfRectHeight + rectTop;
            painter->drawLine(x, yMin, x, yMax);
        };

        int divideCount = (end - start) / rectWidth;

        //        qDebug() << m_peakCache.count() << divideCount;
        for (int i = 0; i < rectWidth; i++) {
            short min = 0;
            short max = 0;
            auto updateMinMax = [](const std::tuple<short, short> &frame, short &min, short &max) {
                auto frameMin = std::get<0>(frame);
                auto frameMax = std::get<1>(frame);
                if (frameMin < min)
                    min = frameMin;
                if (frameMax > max)
                    max = frameMax;
            };

            for (int j = start + i * divideCount; j < (start + i * divideCount + divideCount); j++) {
                auto frame = m_peakCache.at(j);
                updateMinMax(frame, min, max);
            }
            if (i == rectWidth - 1) {
                for (int j = start + i * (divideCount + 1); j < m_peakCache.count(); j++) {
                    auto frame = m_peakCache.at(j);
                    updateMinMax(frame, min, max);
                }
            }
            drawPeak(i, min, max);
        }
    };

    // TODO: waveform shown in line and individual sample mode
    switch (m_renderMode) {
        case Peak:
            drawPeakGraphic();
            break;
        case Line:
            break;
        case Sample:
            break;
    }
}