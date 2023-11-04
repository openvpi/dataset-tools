//
// Created by fluty on 2023/11/3.
//

#include <QDebug>
#include <QPainter>
#include <QSharedPointer>
#include <sndfile.hh>

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
    pen.setWidthF(2);
    painter.setPen(pen);

    painter.setBrush(colorPrimary);
    painter.drawRoundedRect(rect(), 6, 6);
    painter.setBrush(Qt::NoBrush);

    pen.setColor(Qt::white);
    pen.setWidthF(1.5);
    painter.setPen(pen);

    auto font = QFont("Microsoft Yahei UI");
    font.setPointSizeF(10);
    painter.setFont(font);
    int padding = 2;
    auto textRectLeft = rect().left() + padding;
    auto textRectTop = rect().top() + padding;
    auto textRectWidth = rect().width() - 2 * padding;
    auto textRectHeight = rect().height() - 2 * padding;
    auto textRect = QRectF(textRectLeft, textRectTop, textRectWidth, textRectHeight);
    auto fontMetrics = painter.fontMetrics();
    auto textHeight = fontMetrics.height();
    painter.drawText(textRect, m_clipName, QTextOption(Qt::AlignTop));

    if (m_peakCache.count() > 0) {
        //        auto sceneWidth = rectWidth * 1;
        //        auto start = rectWidth / 2;
        //        auto end = rectWidth;
        m_sceneWidth = rectWidth * m_scale;
        auto start = m_renderStart;
        auto end = m_renderEnd;
        auto drawPeak = [&](int x, double min, double max) {
            auto yMin = -min * halfRectHeight + halfRectHeight;
            auto yMax = -max * halfRectHeight + halfRectHeight;
            painter.drawLine(x, yMin, x, yMax);
        };

        int divideCount = m_peakCache.count() / m_sceneWidth;
        for (int i = 0; i < m_sceneWidth; i++) {
            double min = 0;
            double max = 0;

            for (int j = i * divideCount; j < i * divideCount + divideCount; j++) {
                auto rawFrame = m_peakCache.at(j);
                auto frameMin = std::get<0>(rawFrame);
                auto frameMax = std::get<1>(rawFrame);
                if (frameMin < min)
                    min = frameMin;
                if (frameMax > max)
                    max = frameMax;
            }
            if (i == m_sceneWidth - 1) {
                for (int j = i * (divideCount + 1); j < m_peakCache.count(); j++) {
                    auto rawFrame = m_peakCache.at(j);
                    auto frameMin = std::get<0>(rawFrame);
                    auto frameMax = std::get<1>(rawFrame);
                    if (frameMin < min)
                        min = frameMin;
                    if (frameMax > max)
                        max = frameMax;
                }
            }

            double x = 1.0 * (i - start) / (end - start) * rectWidth;
            drawPeak(x, min, max);
        }
    }

    QFrame::paintEvent(event);
}

bool WaveformWidget::openFile(const QString &path) {

    auto pathStr =
#ifdef Q_OS_WIN
        path.toStdWString();
#else
        path.toStdString();
#endif
    SndfileHandle sf(pathStr.c_str());
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

    m_clipName = path;

    update();

    return true;
}

void WaveformWidget::resizeEvent(QResizeEvent *event) {
//    if (m_rectLastWidth != -1) {
//        if (m_renderEnd < rect().width() * m_scale) {
//            m_renderEnd += (rect().width() - m_rectLastWidth) * m_scale;
//        }
//    }
//
//    if (m_rectLastWidth == -1)
//        m_renderEnd = rect().width();
//
//    m_rectLastWidth = rect().width();
    m_renderEnd = rect().width();

    QWidget::resizeEvent(event);
}

void WaveformWidget::zoomIn(double factor) {
    auto zoomedScale = m_scale * factor;
    if (zoomedScale > 8)
        return;
    m_scale = zoomedScale;
    m_renderEnd /= factor;
    update();
}

void WaveformWidget::zoomOut(double factor) {
    auto zoomedScale = m_scale / factor;
    if (zoomedScale < 1)
        return;
    m_scale = zoomedScale;
    m_renderEnd *= factor;
    update();
}

bool WaveformWidget::eventFilter(QObject *object, QEvent *event) {

    return QObject::eventFilter(object, event);
}

void WaveformWidget::wheelEvent(QWheelEvent *event) {
    //    qDebug() << event->angleDelta();
    auto mousePos = event->position();
    auto angleY = event->angleDelta().y();
    auto factor = 1 + 0.2 * qAbs(angleY) / 120;
    if (angleY > 0) {
        zoomIn(factor);
    } else if (angleY < 0) {
        zoomOut(factor);
    }
    qDebug() << m_scale;
    QWidget::wheelEvent(event);
}

void WaveformWidget::mouseMoveEvent(QMouseEvent *event) {
    auto dx = event->pos().x() - m_mouseLastPos.x();
    //    qDebug() << dx;
    auto scaledDx = dx / m_scale;
    auto start = m_renderStart;
    auto end = m_renderEnd;
    //    qDebug() << start << end << m_sceneWidth;
    if (start - dx > 0) {
        //        m_renderStart -= scaledDx;
        if (end - dx < m_sceneWidth) {
            m_renderStart -= scaledDx;
            m_renderEnd -= scaledDx;
        } else {
            m_renderEnd = m_sceneWidth;
        }
    } else {
        m_renderStart = 0;
    }
    //    if (end - dx < m_sceneWidth)
    //    {
    //        m_renderEnd -= scaledDx;
    //    } else {
    //        m_renderEnd = m_sceneWidth;
    //    }
    m_mouseLastPos = event->pos();

    update();
    QWidget::mouseMoveEvent(event);
}

void WaveformWidget::mousePressEvent(QMouseEvent *event) {
    m_mouseLastPos = event->pos();
    QWidget::mousePressEvent(event);
}
