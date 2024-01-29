//
// Created by fluty on 2023/8/6.
//

#include "LevelMeterChunk.h"
#include <QDebug>
#include <QPainter>
#include <QTimer>

LevelMeterChunk::LevelMeterChunk(QWidget *parent) : QWidget(parent) {
    calculateParams();
    m_timer = new QTimer(parent);
    m_timer->setInterval(16);
    m_timer->start();
    QObject::connect(m_timer, &QTimer::timeout, this, [=]() {
        double sum = 0;
        for (int i = 0; i < m_bufferSize; i++) {
            sum += m_bufferPtr[i];
        }
        averageLevel = sum / m_bufferSize;
        // qDebug() << averageLevel;
        //        repaint();
        update();
    });
}

LevelMeterChunk::~LevelMeterChunk() {
    delete[] m_bufferPtr;
    delete m_timer;
}

void LevelMeterChunk::calculateParams() {
    m_rect = rect();
    m_rectWidth = m_rect.width();
    m_rectLeft = m_rect.left();
    m_rectTop = m_rect.top();
    m_rectHeight = m_rect.height();

    if (m_orientation == Qt::Horizontal) {
        m_levelLength = int(m_rectWidth * averageLevel);
        m_safeLength = int(m_rectWidth * m_safeThreshold);
        m_warnLength = int(m_rectWidth * m_warnThreshold) - m_safeLength;
        m_criticalLength = rect().width() - m_safeLength - m_warnLength;

        m_safeStart = m_rectLeft;
        m_warnStart = m_rectLeft + m_safeLength;
        m_criticalStart = m_rectLeft + m_safeLength + m_warnLength;

        m_safeChunk.setRect(m_safeStart, m_rectTop, m_safeLength, m_rectHeight);
        m_warnChunk.setRect(m_warnStart, m_rectTop, m_warnLength, m_rectHeight);
        m_criticalChunk.setRect(m_criticalStart, m_rectTop, m_criticalLength, m_rectHeight);
    } else {
        m_levelLength = int(m_rectHeight * averageLevel);
        m_safeLength = int(m_rectHeight * m_safeThreshold);
        m_warnLength = int(m_rectHeight * m_warnThreshold) - m_safeLength;
        m_criticalLength = m_rectHeight - m_safeLength - m_warnLength;

        m_safeStart = m_rectHeight - m_safeLength;
        m_warnStart = m_rectHeight - m_safeLength - m_warnLength;
        m_criticalStart = m_rectTop;

        m_safeChunk.setRect(m_rectLeft, m_safeStart, m_rectWidth, m_safeLength);
        m_warnChunk.setRect(m_rectLeft, m_warnStart, m_rectWidth, m_warnLength);
        m_criticalChunk.setRect(m_rectLeft, m_criticalStart, m_rectWidth, m_criticalLength);
    }
}

void LevelMeterChunk::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    // Fill background
    painter.fillRect(rect(), m_colorBackground);
    //    qDebug() << rect();

    auto drawHorizontalBar = [&]() {
        m_levelLength = int(m_rectWidth * averageLevel);
        if (averageLevel < m_safeThreshold) {
            auto width = m_levelLength;
            auto chunk = QRect(m_safeStart, m_rectTop, width, m_rectHeight);
            painter.fillRect(chunk, m_colorSafe);
        } else if (averageLevel < m_warnThreshold) {
            painter.fillRect(m_safeChunk, m_colorSafe);

            auto width = m_levelLength - m_safeLength;
            auto chunk = QRect(m_warnStart, m_rectTop, width, m_rectHeight);
            painter.fillRect(chunk, m_colorWarn);
        } else if (averageLevel < 1) {
            painter.fillRect(m_safeChunk, m_colorSafe);
            painter.fillRect(m_warnChunk, m_colorWarn);

            auto width = m_levelLength - m_safeLength - m_warnLength;
            auto chunk = QRect(m_criticalStart, m_rectTop, width, m_rectHeight);
            painter.fillRect(chunk, m_colorCritical);
        } else {
            painter.fillRect(m_safeChunk, m_colorSafe);
            painter.fillRect(m_warnChunk, m_colorWarn);
            painter.fillRect(m_criticalChunk, m_colorCritical);
        }
    };

    auto drawVerticalBar = [&]() {
        m_levelLength = int(m_rectHeight * averageLevel);
        if (averageLevel < m_safeThreshold) {
            auto height = m_levelLength;
            auto top = m_rectHeight - height;
            auto chunk = QRect(m_rectLeft, top, m_rectWidth, height);
            painter.fillRect(chunk, m_colorSafe);
        } else if (averageLevel < m_warnThreshold) {
            painter.fillRect(m_safeChunk, m_colorSafe);

            auto height = m_levelLength - m_safeLength;
            auto top = m_rectHeight - m_levelLength;
            auto chunk = QRect(m_rectLeft, top, m_rectWidth, height);
            painter.fillRect(chunk, m_colorWarn);
        } else if (averageLevel < 1) {
            painter.fillRect(m_safeChunk, m_colorSafe);
            painter.fillRect(m_warnChunk, m_colorWarn);

            auto height = m_levelLength - m_safeLength - m_warnLength;
            auto top = m_rectHeight - m_levelLength;
            auto chunk = QRect(m_rectLeft, top, m_rectWidth, height);
            painter.fillRect(chunk, m_colorCritical);
        } else {
            painter.fillRect(m_safeChunk, m_colorSafe);
            painter.fillRect(m_warnChunk, m_colorWarn);
            painter.fillRect(m_criticalChunk, m_colorCritical);
        }
    };

    // Calculate and draw chunk
    if (m_orientation == Qt::Horizontal)
        drawHorizontalBar();
    else
        drawVerticalBar();

    painter.end();
    QWidget::paintEvent(event);
}

void LevelMeterChunk::readSample(double sample) {
    //    qDebug() << l;
    m_bufferPtr[m_bufferPos] = sample;
    m_bufferPos++;
    if (m_bufferPos == m_bufferSize) {
        m_bufferPos = 0;
    }
    //    update();
}

// void LevelMeterChunk::setSampleRate(int sampleRate) {
// }

void LevelMeterChunk::setBufferSize(int size) {
    setFreeze(true);
    delete[] m_bufferPtr;
    initBuffer(size);
    setFreeze(false);
}

void LevelMeterChunk::initBuffer(int bufferSize) {
    m_bufferSize = bufferSize;
    m_bufferPtr = new double[m_bufferSize];
    resetBuffer();
}

void LevelMeterChunk::resetBuffer() {
    for (int i = 0; i < m_bufferSize; i++)
        m_bufferPtr[i] = 0;
    m_bufferPos = 0;
}

void LevelMeterChunk::setOrientation(Qt::Orientation orientation) {
    m_orientation = orientation;
    update();
}

void LevelMeterChunk::setValue(double value) {
    averageLevel = value;
    repaint();
}

void LevelMeterChunk::resizeEvent(QResizeEvent *event) {
    calculateParams();
    QWidget::resizeEvent(event);
}

void LevelMeterChunk::setFreeze(bool on) {
    if (on) {
        m_timer->stop();
        m_freeze = true;
    } else {
        m_timer->start();
        m_freeze = false;
    }
}

int LevelMeterChunk::bufferSize() const {
    return m_bufferSize;
}

bool LevelMeterChunk::freeze() const {
    return m_freeze;
}
