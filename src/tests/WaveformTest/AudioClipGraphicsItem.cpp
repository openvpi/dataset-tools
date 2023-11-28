//
// Created by fluty on 2023/11/16.
//

#include <QPainter>
#include <QDebug>

#include "AudioClipGraphicsItem.h"


AudioClipGraphicsItem::AudioClipGraphicsItem(QGraphicsItem *parent) {
}
AudioClipGraphicsItem::~AudioClipGraphicsItem() {
}
bool AudioClipGraphicsItem::openFile(const QString &path) {
    auto pathStr =
#ifdef Q_OS_WIN
        path.toStdWString();
#else
            path.toStdString();
#endif
    //    SndfileHandle sf(pathStr.c_str());
    sf = SndfileHandle(pathStr.c_str());
    auto sfErrCode = sf.error();
    auto sfErrMsg = sf.strError();

    if (sfErrCode) {
        qDebug() << sfErrMsg;
        return false;
    }

    QVector<std::tuple<double, double>> nullVector;
    m_peakCache.swap(nullVector);

    auto sr = sf.samplerate();
    auto channels = sf.channels();
    auto frames = sf.frames();
    auto totalSize = frames * channels;

    int chunkSize = 512;
    std::vector<double> buffer(chunkSize * channels);
    qint64 samplesRead = 0;
    while (samplesRead < frames * channels) {
        samplesRead = sf.read(buffer.data(), chunkSize * channels);
        if (samplesRead == 0) {
            break;
        }
        double max = 0;
        double min = 0;
        qint64 framesRead = samplesRead / channels;
        for (qint64 i = 0; i < framesRead; i++) {
            double monoSample = 0.0;
            for (int j = 0; j < channels; j++) {
                monoSample += buffer[i * channels + j] / static_cast<double>(channels);
            }
            if (monoSample > max)
                max = monoSample;
            if (monoSample < min)
                min = monoSample;
            auto pair = std::make_pair(min, max);
            m_peakCache.append(pair);
        }
    }

    setName(path);
    m_renderStart = 0;
    setClipStart(0);
    m_renderEnd = m_peakCache.count();
    setLength(m_renderEnd / chunksPerTick);
    setClipLen(m_renderEnd / chunksPerTick);
    m_scale = 1.0;

    update();

    return true;
}
void AudioClipGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    // Draw frame
    ClipGraphicsItem::paint(painter, option, widget);

    painter->setRenderHint(QPainter::Antialiasing, false);

    // Draw waveform in previewRect() when file loaded

    // painter->setPen(Qt::NoPen);
    // painter->setBrush(QColor(255, 255, 255, 80));
    // painter->drawRect(previewRect());
    // painter->setPen(Qt::white);
    // painter->drawText(previewRect(), "Waveform", QTextOption(Qt::AlignCenter));

    auto rectLeft = previewRect().left();
    auto rectTop = previewRect().top();
    auto rectWidth = previewRect().width();
    auto rectHeight = previewRect().height();
    auto halfRectHeight = rectHeight / 2;
    auto colorPrimary = QColor(112, 156, 255);
    auto colorPrimaryDarker = QColor(81, 135, 255);
    auto colorAccent = QColor(255, 175, 95);
    auto colorAccentDarker = QColor(255, 159, 63);

    QPen pen;
    pen.setColor(colorPrimary);
    pen.setWidthF(2);
    painter->setPen(pen);

    pen.setColor(Qt::white);
    pen.setWidthF(1.5);
    painter->setPen(pen);

    auto drawPeakGraphic = [&](){
        if (m_peakCache.count() == 0)
            return;

        // auto start = m_renderStart;
        // auto end = m_renderEnd;
        auto start = clipStart() * chunksPerTick;
        auto end = (clipStart() + clipLen()) * chunksPerTick;
        auto drawPeak = [&](int x, double min, double max) {
            auto yMin = -min * halfRectHeight + halfRectHeight + rectTop;
            auto yMax = -max * halfRectHeight + halfRectHeight + rectTop;
            painter->drawLine(x, yMin, x, yMax);
        };

        int divideCount = (end - start) / rectWidth;
        //        qDebug() << m_peakCache.count() << divideCount;
        for (int i = 0; i < rectWidth; i++) {
            double min = 0;
            double max = 0;
            auto updateMinMax = [](const std::tuple<double, double> &frame, double &min, double &max) {
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