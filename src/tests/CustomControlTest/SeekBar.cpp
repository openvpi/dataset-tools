//
// Created by fluty on 2023/8/16.
//

#include "SeekBar.h"
#include <QDebug>
#include <QPainter>
#include <QTimer>

class SeekBarPrivate {
public:
    SeekBar *q;

    explicit SeekBarPrivate(SeekBar *q) : q(q) {
        q->setMinimumHeight(24);
        q->setMaximumHeight(24);
    }
};

SeekBar::SeekBar(QWidget *parent) : QWidget(parent), d(new SeekBarPrivate(this)) {
    timer = new QTimer(parent);
    timer->setInterval(200);
    QObject::connect(timer, &QTimer::timeout, this, [=]() {
        timer->stop();
        doubleClickLocked = false;
    });
    this->setMinimumWidth(50);
}

SeekBar::~SeekBar() {
    delete d;
}

void SeekBar::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    // Fill background
    // painter.fillRect(rect(), QColor("#d9d9d9"));

    // Calculate slider track inactive(background)
    actualLeft = rect().left() + 16;
    actualRight = rect().right() - 16;
    actualWidth = rect().width() - 16 - 16;
    auto trackLeft = QPoint(actualLeft, rect().height() / 2);
    auto trackRight = QPoint(actualRight, rect().height() / 2);

    // Draw slider track inactive(background)
    QPen pen;
    pen.setColor(QColor(217, 217, 217));
    pen.setCapStyle(Qt::RoundCap);
    pen.setWidth(8);
    painter.setPen(pen);
    painter.drawLine(trackLeft, trackRight);

    // Calculate slider track active
    auto activeStartPos = int(actualWidth * (m_trackActiveStartValue - m_min) / (m_max - m_min)) + 16;
    auto activePoint1 = QPoint(activeStartPos, rect().height() / 2);
    auto valuePos = int(actualWidth * (m_value - m_min) / (m_max - m_min)) + 16;
    auto activePoint2 = QPoint(valuePos, rect().height() / 2);

    // Draw slider track active
    pen.setColor(QColor(112, 156, 255));
    pen.setWidth(8);
    painter.setPen(pen);
    painter.drawLine(activePoint1, activePoint2);

    // Draw handle
    //    auto handleTopLeft = QPoint(rect().width() / 2 + rect().width() / 3 - 8, rect().height() / 2 - 8);
    //    auto handleBottomRight = QPoint(rect().width() / 2 + rect().width() / 3 + 8, rect().height() / 2 + 8);
    auto handlePos = activePoint2;
    auto handleR = 8;
    pen.setColor(QColor(255, 255, 255));
    pen.setWidth(4);
    painter.setPen(pen);
    painter.setBrush(QColor(112, 156, 255));
    painter.drawEllipse(handlePos, handleR, handleR);

    painter.end();
}

void SeekBar::setValue(double value) {
    m_value = value;
    update();
}

void SeekBar::setDefaultValue(double value) {
    m_defaultValue = value;
    update();
}

void SeekBar::setMax(double max) {
    m_max = max;
    update();
}

void SeekBar::setMin(double min) {
    m_min = min;
    update();
}

void SeekBar::reset() {
    m_value = m_defaultValue;
    update();
    emit valueChanged(m_value);
}

void SeekBar::mouseMoveEvent(QMouseEvent *event) {
    auto pos = event->pos();
    if (pos.x() < actualLeft) {
        setValue(m_min);
        emit valueChanged(m_value);
    }
    else if (pos.x() < actualRight) {
        //        auto valuePos = actualWidth * (m_value - m_min) / (m_max - m_min) + 16;
        auto posValue = (pos.x() - 16) * (m_max - m_min) / actualWidth + m_min;
        setValue(posValue);
        emit valueChanged(m_value);
    } else {
        setValue(m_max);
        emit valueChanged(m_value);
    }
    QWidget::mouseMoveEvent(event);
}

void SeekBar::mouseDoubleClickEvent(QMouseEvent *event) {
    //    qDebug() << "double click";
    auto pos = event->pos();
    if (mouseOnHandle(pos))
        reset();
    QWidget::mouseDoubleClickEvent(event);
}

void SeekBar::mousePressEvent(QMouseEvent *event) {
    //    qDebug() << "press";
    timer->start();
    auto pos = event->pos();
    if (!mouseOnHandle(pos) && !doubleClickLocked) {
        if (pos.x() >= actualLeft && pos.x() <= actualRight) {
            //        auto valuePos = actualWidth * (m_value - m_min) / (m_max - m_min) + 16;
            auto posValue = (pos.x() - 16) * (m_max - m_min) / actualWidth + m_min;
            setValue(posValue);
            emit valueChanged(m_value);
        }
    }
    doubleClickLocked = true;
    QWidget::mousePressEvent(event);
}

void SeekBar::mouseReleaseEvent(QMouseEvent *event) {
    QWidget::mouseReleaseEvent(event);
}

bool SeekBar::mouseOnHandle(const QPoint &mousePos) const {
    auto pos = mousePos;
    auto valuePos = actualWidth * (m_value - m_min) / (m_max - m_min) + 16;
    if (pos.x() >= valuePos - 8 - 4 && pos.x() <= valuePos + 8 + 4) {
        return true;
    }
    return false;
}

void SeekBar::setTrackActiveStartValue(double pos) {
    m_trackActiveStartValue = pos;
    update();
}

void SeekBar::setRange(double min, double max) {
    setMin(min);
    setMax(max);
}
