//
// Created by fluty on 2023/8/14.
//

#include <QPainter>
#include <QPropertyAnimation>
#include <QTimer>
#include <QDebug>

#include "ProgressIndicator.h"

ProgressIndicator::ProgressIndicator(QWidget *parent) : QWidget(parent) {
    initUi(parent);
}

ProgressIndicator::ProgressIndicator(ProgressIndicator::IndicatorStyle indicatorStyle, QWidget *parent) {
    m_indicatorStyle = indicatorStyle;
    initUi(parent);
}

ProgressIndicator::~ProgressIndicator() {
}

void ProgressIndicator::initUi(QWidget *parent) {
//    qDebug() << "init";
    m_timer = new QTimer(parent);
    m_timer->setInterval(8);
    m_timer->connect(m_timer, &QTimer::timeout, this, [=]() {
        setThumbProgress(m_thumbProgress + 1);
        if (m_thumbProgress == 240)
            m_thumbProgress = 0;
    });
    m_colorPalette = colorPaletteNormal;

//    m_animation = new QPropertyAnimation;
//    m_animation->setTargetObject(this);
//    m_animation->setPropertyName("thumbProgress");
//    m_animation->setDuration(2000);
//    m_animation->setStartValue(0);
//    m_animation->setEndValue(240);
//    m_animation->setLoopCount(-1);
    switch (m_indicatorStyle) {
        case HorizontalBar:
            this->setMinimumHeight(8);
            this->setMaximumHeight(8);
        case Ring:
            break;
    }
}

void ProgressIndicator::paintEvent(QPaintEvent *event) {
//    double penWidth = 8;
    int penWidth = rect().height();
    int padding = penWidth / 2;
    int halfRectHeight = rect().height() / 2;

//    qDebug() << "paint";
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen;

    // Fill background
//    painter.fillRect(rect(), QColor("#808080"));

    // Calculate track inactive(background)
    auto trackStart = QPoint(rect().left() + padding, halfRectHeight);
    auto trackEnd = QPoint(rect().right() - padding, halfRectHeight);
    auto actualLength = rect().width() - padding - padding;

    auto drawBarBackground = [&]() {
        // Draw track inactive(background)
        pen.setColor(m_colorPalette.inactive);
        pen.setCapStyle(Qt::RoundCap);
        pen.setWidth(penWidth);
        painter.setPen(pen);
        painter.drawLine(trackStart, trackEnd);
    };

    auto drawNormalProgress = [&]() {
        // Calculate secondary progress value
        auto valueStartPos = padding;
        auto valueStartPoint = QPoint(valueStartPos, halfRectHeight);
        auto secondaryValuePos = int(actualLength * (m_secondaryValue - m_min) / (m_max - m_min)) + padding;
        auto secondaryValuePoint2 = QPoint(secondaryValuePos, halfRectHeight);

        // Draw secondary progress value
        pen.setColor(m_colorPalette.secondary);
        pen.setWidth(penWidth);
        painter.setPen(pen);
        painter.drawLine(valueStartPoint, secondaryValuePoint2);

        // Calculate progress value
        auto valueLength = int(actualLength * (m_value - m_min) / (m_max - m_min));
        auto valuePos = valueLength + padding;
        auto valuePoint = QPoint(valuePos, halfRectHeight);

        // Draw progress value
        pen.setColor(m_colorPalette.total);
        pen.setWidth(penWidth);
        painter.setPen(pen);
        painter.drawLine(valueStartPoint, valuePoint);

        // Calculate current task progress value
        auto curTaskValuePos = int(valueLength * (m_currentTaskValue - m_min) / (m_max - m_min)) + padding;
        auto curTaskValuePoint = QPoint(curTaskValuePos, halfRectHeight);

        // Draw current task progress value
        pen.setColor(m_colorPalette.currentTask);
        pen.setWidth(penWidth);
        painter.setPen(pen);
        painter.drawLine(valueStartPoint, curTaskValuePoint);
    };

    auto drawIndeterminateProgress = [&](){
        // Calculate progress value
        auto thumbLength = rect().width() / 3;
        auto thumbActualRight = qRound(m_thumbProgress * (actualLength + thumbLength) / 240.0) + padding;
        auto thumbActualLeft = thumbActualRight - thumbLength;
        QPoint point1;
        if (thumbActualLeft < padding)
            point1 = QPoint(padding, halfRectHeight);
        else
            point1 = QPoint(thumbActualLeft, halfRectHeight);

        QPoint point2;
        auto trackActualRight = trackEnd.x();
        if (thumbActualRight < padding)
            point2 = QPoint(padding, halfRectHeight);
        else if (thumbActualRight < trackActualRight)
            point2 = QPoint(thumbActualRight, halfRectHeight);
        else
            point2 = QPoint(trackActualRight, halfRectHeight);

        // Draw progress value
        pen.setColor(m_colorPalette.total);
        pen.setWidth(penWidth);
        painter.setPen(pen);
//        qDebug() << point1 << point2;
        painter.drawLine(point1, point2);

//        if (m_indeterminateThumbX - thumbLength > trackActualRight)
//            m_indeterminateThumbX = 0; // reset thumb pos
    };

    if (m_indicatorStyle == HorizontalBar) {
        drawBarBackground();
        if (!m_indeterminate)
            drawNormalProgress();
        else
            drawIndeterminateProgress();
    }

    painter.end();
}

double ProgressIndicator::value() const {
    return m_value;
}

void ProgressIndicator::setValue(double value) {
    m_value = value;
    update();
}

double ProgressIndicator::minimum() const {
    return m_min;
}

void ProgressIndicator::setMinimum(double minimum) {
    m_min = minimum;
    update();
}

double ProgressIndicator::maximum() const {
    return m_max;
}

void ProgressIndicator::setMaximum(double maximum) {
    m_max = maximum;
    update();
}

void ProgressIndicator::setRange(double minimum, double maximum) {
    m_min = minimum;
    m_max = maximum;
    update();
}

double ProgressIndicator::secondaryValue() const {
    return m_secondaryValue;
}

void ProgressIndicator::setSecondaryValue(double value) {
    m_secondaryValue = value;
    update();
}

void ProgressIndicator::setCurrentTaskValue(double value) {
    m_currentTaskValue = value;
    update();
}

void ProgressIndicator::reset() {
    m_value = 0;
    m_secondaryValue = 0;
    m_currentTaskValue = 0;
    update();
}

double ProgressIndicator::currentTaskValue() const {
    return m_currentTaskValue;
}

bool ProgressIndicator::indeterminate() const {
    return m_indeterminate;
}

void ProgressIndicator::setIndeterminate(bool on) {
    m_indeterminate = on;
    if (m_indeterminate)
//        m_animation->start();
        m_timer->start();
    else
//        m_animation->stop();
        m_timer->stop();
    update();
}

int ProgressIndicator::thumbProgress() const {
    return m_thumbProgress;
}

void ProgressIndicator::setThumbProgress(int x) {
    m_thumbProgress = x;
//    qDebug() << x;
    repaint();
}

ProgressIndicator::TaskStatus ProgressIndicator::taskStatus() const {
    return m_taskStatus;
}

void ProgressIndicator::setTaskStatus(ProgressIndicator::TaskStatus status) {
    m_taskStatus = status;
    switch (m_taskStatus) {
        case Normal:
            m_colorPalette = colorPaletteNormal;
            return;
        case Warning:
            m_colorPalette = colorPaletteWarning;
            break;
        case Error:
            m_colorPalette = colorPaletteError;
            break;
    }
    update();
}
