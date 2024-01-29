//
// Created by fluty on 2023/8/16.
//

#include "SeekBar.h"
#include <QDebug>
#include <QPainter>
#include <QTimer>

SeekBar::SeekBar(QWidget *parent) : QWidget(parent) {
    timer = new QTimer(parent);
    timer->setInterval(200);
    QObject::connect(timer, &QTimer::timeout, this, [=]() {
        timer->stop();
        doubleClickLocked = false;
    });
    this->setMinimumWidth(50);
    setMinimumHeight(20);
    setMaximumHeight(20);
    setAttribute(Qt::WA_Hover, true);
    installEventFilter(this);
    m_thumbHoverAnimation = new QPropertyAnimation;
    m_thumbHoverAnimation->setTargetObject(this);
    m_thumbHoverAnimation->setPropertyName("thumbBorderRatio");
    m_thumbHoverAnimation->setDuration(200);
    m_thumbHoverAnimation->setEasingCurve(QEasingCurve::OutCubic);
    calculateParams();
}
double SeekBar::value() {
    return m_value;
}

void SeekBar::calculateParams() {
    m_rect = rect();
    m_halfHeight = m_rect.height() / 2;
    m_padding = m_halfHeight;
    m_trackPenWidth = rect().height() / 3;

    // Calculate slider track
    m_actualStart = m_rect.left() + m_padding;
    m_actualEnd = m_rect.right() - m_padding;
    m_actualLength = m_rect.width() - 2 * m_padding;
    m_trackStartPoint.setX(m_actualStart);
    m_trackStartPoint.setY(m_halfHeight);
    m_trackEndPoint.setX(m_actualEnd);
    m_trackEndPoint.setY(m_halfHeight);

    // Calculate slider track active
    m_activeStartPos = int(m_actualLength * (m_trackActiveStartValue - m_min) / (m_max - m_min)) + m_padding;
    m_activeStartPoint.setX(m_activeStartPos);
    m_activeStartPoint.setY(m_halfHeight);
    m_valuePos = int(m_actualLength * (m_value - m_min) / (m_max - m_min)) + m_padding;
    m_activeEndPoint.setX(m_valuePos);
    m_activeEndPoint.setY(m_halfHeight);
}

void SeekBar::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    // Fill background
    //     painter.fillRect(rect(), QColor("#d0d0d0"));

    // Draw slider track inactive(background)
    QPen pen;
    pen.setColor(m_trackInactiveColor);
    pen.setCapStyle(Qt::RoundCap);
    pen.setWidth(m_trackPenWidth);
    painter.setPen(pen);
    painter.drawLine(m_trackStartPoint, m_trackEndPoint);

    m_valuePos = int(m_actualLength * (m_value - m_min) / (m_max - m_min)) + m_padding;
    m_activeEndPoint.setX(m_valuePos);

    // Draw slider track active
    pen.setColor(m_trackActiveColor);
    pen.setWidth(m_trackPenWidth);
    painter.setPen(pen);
    painter.drawLine(m_activeStartPoint, m_activeEndPoint);

    // Draw handle

    // Calculate handle
    m_handlePenWidth = m_halfHeight * m_thumbBorderRatio / 255.0;
    //    qDebug() << m_thumbBorderRatio;
    m_handleRadius = m_halfHeight - m_handlePenWidth;
    pen.setColor(m_thumbBorderColor);
    pen.setWidthF(m_handlePenWidth);
    painter.setPen(pen);
    painter.setBrush(m_trackActiveColor);
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
    } else if (pos.x() < m_actualEnd) {
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
    m_thumbHoverAnimation->stop();
    m_thumbHoverAnimation->setStartValue(m_thumbBorderRatio);
    m_thumbHoverAnimation->setEndValue(114);
    m_thumbHoverAnimation->start();
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
    //    qDebug() << "release";
    m_thumbHoverAnimation->stop();
    m_thumbHoverAnimation->setStartValue(m_thumbBorderRatio);
    m_thumbHoverAnimation->setEndValue(77);
    m_thumbHoverAnimation->start();
    QWidget::mouseReleaseEvent(event);
}

bool SeekBar::mouseOnHandle(const QPoint &mousePos) const {
    auto pos = mousePos;
    auto valuePos = m_actualLength * (m_value - m_min) / (m_max - m_min) + m_padding;
    if (pos.x() >= valuePos - m_halfHeight && pos.x() <= valuePos + m_halfHeight) {
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

QColor SeekBar::trackInactiveColor() const {
    return m_trackInactiveColor;
}

void SeekBar::setTrackInactiveColor(const QColor &color) {
    m_trackInactiveColor = color;
    update();
}

QColor SeekBar::trackActiveColor() const {
    return m_trackActiveColor;
}

void SeekBar::setTrackActiveColor(const QColor &color) {
    m_trackActiveColor = color;
    update();
}

QColor SeekBar::thumbBorderColor() const {
    return m_thumbBorderColor;
}

void SeekBar::setThumbBorderColor(const QColor &color) {
    m_thumbBorderColor = color;
    update();
}

bool SeekBar::eventFilter(QObject *object, QEvent *event) {
    if (event->type() == QEvent::HoverEnter) {
        //        qDebug() << "Hover Enter";
        m_thumbHoverAnimation->stop();
        m_thumbHoverAnimation->setStartValue(m_thumbBorderRatio);
        m_thumbHoverAnimation->setEndValue(77);
        m_thumbHoverAnimation->start();
    } else if (event->type() == QEvent::HoverLeave) {
        //        qDebug() << "Hover Leave";
        m_thumbHoverAnimation->stop();
        m_thumbHoverAnimation->setStartValue(m_thumbBorderRatio);
        m_thumbHoverAnimation->setEndValue(102);
        m_thumbHoverAnimation->start();
    }
    return QObject::eventFilter(object, event);
}

int SeekBar::thumbBorderRatio() const {
    return m_thumbBorderRatio;
}

void SeekBar::setThumbBorderRatio(int ratio) {
    m_thumbBorderRatio = ratio;
    update();
}
