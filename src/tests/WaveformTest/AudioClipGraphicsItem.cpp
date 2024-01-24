//
// Created by fluty on 2023/11/16.
//

#include <QDebug>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QPainter>
#include <QThread>

#include "AudioClipBackgroundWorker.h"
#include "AudioClipGraphicsItem.h"

AudioClipGraphicsItem::AudioClipGraphicsItem(int itemId, QGraphicsItem *parent) : ClipGraphicsItem(itemId, parent) {
    m_clipTypeStr = "[Audio] ";
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
        m_loading = false;
        qDebug() << "open file error" << errorMessage;
        return;
    }
    m_sampleRate = m_worker->sampleRate;
    m_channels = m_worker->channels;
    m_chunkSize = m_worker->chunkSize;
    m_mipmapScale = m_worker->mipmapScale;

    m_peakCache.swap(m_worker->peakCache);
    m_peakCacheMipmap.swap(m_worker->peakCacheMipmap);
    delete m_worker;

    setClipStart(0);
    updateLength();
    setClipLen(static_cast<int>(m_peakCache.count() / m_chunksPerTick));
    m_loading = false;

    update();
}
void AudioClipGraphicsItem::onTempoChange(double tempo) {
    m_tempo = tempo;
    updateLength();
}
void AudioClipGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    QElapsedTimer mstimer;
    // Draw frame
    ClipGraphicsItem::paint(painter, option, widget);

    painter->setRenderHint(QPainter::Antialiasing, false);

    auto rectLeft = previewRect().left();
    auto rectTop = previewRect().top();
    auto rectWidth = previewRect().width();
    auto rectHeight = previewRect().height();
    auto halfRectHeight = rectHeight / 2;
    if (rectHeight < 32 || rectWidth < 16)
        return;
    auto colorAlpha = rectHeight <= 48 ? 255 * (rectHeight - 32) / (48 - 32) : 255;
    auto peakColor = QColor(10, 10, 10, static_cast<int>(colorAlpha));

    QPen pen;

    if (m_loading) {
        pen.setColor(peakColor);
        painter->setPen(pen);
        painter->drawText(previewRect(), "Loading...", QTextOption(Qt::AlignCenter));
    }

    pen.setColor(peakColor);
    pen.setWidth(1);
    painter->setPen(pen);

    mstimer.start();
    if (m_peakCache.count() == 0 || m_peakCacheMipmap.count() == 0)
        return;

    m_resolution = scaleX() >= 0.3 ? High : Low;
    const auto peakData = m_resolution == Low ? m_peakCacheMipmap : m_peakCache;
    const auto chunksPerTick = m_resolution == Low ? m_chunksPerTick / m_mipmapScale : m_chunksPerTick;

    auto rectLeftScene = mapToScene(previewRect().topLeft()).x();
    auto rectRightScene = mapToScene(previewRect().bottomRight()).x();
    auto waveRectLeft = m_visibleRect.left() < rectLeftScene ? 0 : m_visibleRect.left() - rectLeftScene;
    auto waveRectRight =
        m_visibleRect.right() < rectRightScene ? m_visibleRect.right() - rectLeftScene : rectRightScene - rectLeftScene;
    auto waveRectWidth = waveRectRight - waveRectLeft;

    auto start = clipStart() * chunksPerTick;
    auto end = (clipStart() + clipLen()) * chunksPerTick;
    auto divideCount = ((end - start) / rectWidth);
    // qDebug() << start << end << rectWidth << (int)divideCount;

    auto drawPeak = [&](int x, short min, short max) {
        auto yMin = -min * halfRectHeight / 32767 + halfRectHeight + rectTop;
        auto yMax = -max * halfRectHeight / 32767 + halfRectHeight + rectTop;
        painter->drawLine(x, static_cast<int>(yMin), x, static_cast<int>(yMax));
    };

    // qDebug() << m_peakCacheThumbnail.count() << divideCount;
    for (int i = static_cast<int>(waveRectLeft); i <= static_cast<int>(waveRectRight); i++) {
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
            if (j >= peakData.count())
                break;

            const auto &frame = peakData.at(j);
            updateMinMax(frame, min, max);
        }
        // if (i == rectWidth - 1) {
        //     // for (int j = start + i * (divideCount + 1); j < m_peakCache.count(); j++) {
        //     for (int j = start + i * (divideCount + 1); j < peakData.count(); j++) {
        //         // auto frame = m_peakCache.at(j);
        //         auto frame = peakData.at(j);
        //         updateMinMax(frame, min, max);
        //     }
        // }
        drawPeak(i, min, max);
    }

    const auto time = static_cast<double>(mstimer.nsecsElapsed()) / 1000000.0;
    // qDebug() << time;

    // // Draw visible area
    // auto rectLeftScene = mapToScene(previewRect().topLeft()).x();
    // auto rectRightScene = mapToScene(previewRect().bottomRight()).x();
    // auto waveRectLeft =
    //     m_visibleRect.left() < rectLeftScene ? 0 : m_visibleRect.left() - rectLeftScene;
    // auto waveRectTop = previewRect().top();
    // auto waveRectRight =
    //     m_visibleRect.right() < rectRightScene ? m_visibleRect.right() - rectLeftScene: rectRightScene-
    //     rectLeftScene;
    // auto waveRectWidth = waveRectRight - waveRectLeft;
    // auto waveRectHeight = previewRect().height();
    // auto waveRect = QRectF(waveRectLeft, waveRectTop, waveRectWidth, waveRectHeight);
    //
    // pen.setColor(QColor(255, 0, 0));
    // pen.setWidth(1);
    // painter->setPen(pen);
    // painter->drawRect(waveRect);
    // painter->drawLine(waveRect.topLeft(), waveRect.bottomRight());
    // painter->drawLine(waveRect.topRight(), waveRect.bottomLeft());
}
void AudioClipGraphicsItem::updateLength() {
    m_chunksPerTick = static_cast<double>(m_sampleRate) / m_chunkSize * 60 / m_tempo / 480;
    setLength(static_cast<int>(m_peakCache.count() / m_chunksPerTick));
}