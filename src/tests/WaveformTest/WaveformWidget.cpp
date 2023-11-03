//
// Created by fluty on 2023/11/3.
//

#include <QDebug>
#include <QPainter>
#include <sndfile.hh>
#include <QSharedPointer>

#include "WaveformWidget.h"

WaveformWidget::WaveformWidget(QWidget *parent) {
}

WaveformWidget::~WaveformWidget() {
}

void WaveformWidget::paintEvent(QPaintEvent *event) {
    auto rectWidth = rect().width();
    auto rectHeight = rect().height();
    auto halfRectHeight = rectHeight / 2;
    auto colorPrimary = QColor(112, 156, 255);
    auto colorPrimaryDarker = QColor(81, 135, 255);
    auto colorAccent = QColor(255, 175, 95);
    auto colorAccentDarker = QColor(255, 159, 63);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QPen pen;
    pen.setColor(colorPrimary);
    pen.setWidthF(1.5);
    painter.setPen(pen);

    if (m_peakCache.count() > 0)
    {
        for (int i = 0; i < rectWidth; i++) {
            auto x = i;
            double ratio = 1.0 * i / rectWidth;
            int index = int((m_peakCache.count() - 1) * ratio);

            auto frame = index < m_peakCache.count() ? m_peakCache.at(index) : m_peakCache.last();
            auto yMin = -std::get<0>(frame) * halfRectHeight + halfRectHeight;
            auto yMax = -std::get<1>(frame) * halfRectHeight + halfRectHeight;
            painter.drawLine(x, yMin, x, yMax);
        }
    }

    QFrame::paintEvent(event);
}

bool WaveformWidget::openFile(const QString &path) {

    SndfileHandle sf(path.toStdString());
    auto sfErrCode = sf.error();
    auto sfErrMsg = sf.strError();

    if (sfErrCode) {
        qDebug() << sfErrMsg;
        return false;
    }

    auto sr = sf.samplerate();
    auto channels = sf.channels();
    auto frames = sf.frames();
    auto totalSize = frames * channels;

    std::vector<double> buffer(1024 * channels);
    qint64 samplesRead = 0;
    while (samplesRead < frames * channels) {
        samplesRead = sf.read(buffer.data(), 1024 * channels);
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
    update();

    return true;
}
