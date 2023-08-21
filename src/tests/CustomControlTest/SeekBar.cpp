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
        q->setMinimumHeight(22);
        q->setMaximumHeight(22);
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
    calculateParams();
}

SeekBar::~SeekBar() {
    delete d;
}

void SeekBar::calculateParams() {
    m_rect = rect();
    int halfHeight = m_rect.height() / 2;
    m_padding = halfHeight;
    m_trackPenWidth = rect().height() / 3;

    // Calculate slider track
    m_actualStart = m_rect.left() + m_padding;
    m_actualEnd = m_rect.right() - m_padding;
    m_actualLength = m_rect.width() - 2 * m_padding;
    m_trackStartPoint.setX(m_actualStart);
    m_trackStartPoint.setY(halfHeight);
    m_trackEndPoint.setX(m_actualEnd);
    m_trackEndPoint.setY(halfHeight);

    // Calculate slider track active
    m_activeStartPos = int(m_actualLength * (m_trackActiveStartValue - m_min) / (m_max - m_min)) + m_padding;
    m_activeStartPoint.setX(m_activeStartPos);
    m_activeStartPoint.setY(halfHeight);
    m_valuePos = int(m_actualLength * (m_value - m_min) / (m_max - m_min)) + m_padding;
    m_activeEndPoint.setX(m_valuePos);
    m_activeEndPoint.setY(halfHeight);

    // Calculate handle
    m_handlePenWidth = int(halfHeight * 0.4);
    m_handleRadius = halfHeight - m_handlePenWidth;
}

void SeekBar::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    // Fill background
//     painter.fillRect(rect(), QColor("#d0d0d0"));

    // Draw slider track inactive(background)
    QPen pen;
    pen.setColor(QColor(217, 217, 217));
    pen.setCapStyle(Qt::RoundCap);
    pen.setWidth(m_trackPenWidth);
    painter.setPen(pen);
    painter.drawLine(m_trackStartPoint, m_trackEndPoint);

    m_valuePos = int(m_actualLength * (m_value - m_min) / (m_max - m_min)) + m_padding;
    m_activeEndPoint.setX(m_valuePos);

    // Draw slider track active
    pen.setColor(QColor(112, 156, 255));
    pen.setWidth(m_trackPenWidth);
    painter.setPen(pen);
    painter.drawLine(m_activeStartPoint, m_activeEndPoint);

    // Draw handle
    pen.setColor(QColor(255, 255, 255));
    pen.setWidth(m_handlePenWidth);
    painter.setPen(pen);
    painter.setBrush(QColor(112, 156, 255));
    painter.drawEllipse(m_activeEndPoint, m_handleRadius, m_handleRadius);

    painter.end();
}

void SeekBar::setValue(double value) {
    m_value = value;
    repaint();
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
    if (pos.x() < m_actualStart) {
        setValue(m_min);
        emit valueChanged(m_value);
    }
    else if (pos.x() < m_actualEnd) {
        //        auto valuePos = m_actualLength * (m_value - m_min) / (m_max - m_min) + 16;
        auto posValue = (pos.x() - m_padding) * (m_max - m_min) / m_actualLength + m_min;
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
        auto x = pos.x();
        if (x < m_actualStart) {
            setValue(m_min);
        } else if (x < m_actualEnd) {
            //        auto valuePos = m_actualLength * (m_value - m_min) / (m_max - m_min) + 16;
            auto posValue = (pos.x() - m_padding) * (m_max - m_min) / m_actualLength + m_min;
            setValue(posValue);
        } else {
            setValue(m_max);
        }
        emit valueChanged(m_value);
    }
    doubleClickLocked = true;
}

void SeekBar::mouseReleaseEvent(QMouseEvent *event) {
    QWidget::mouseReleaseEvent(event);
}

bool SeekBar::mouseOnHandle(const QPoint &mousePos) const {
    auto pos = mousePos;
    auto valuePos = m_actualLength * (m_value - m_min) / (m_max - m_min) + 16;
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

void SeekBar::resizeEvent(QResizeEvent *event) {
    calculateParams();
}
