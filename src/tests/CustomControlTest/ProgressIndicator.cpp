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
        setThumbProgress(m_thumbProgress + 2);
        if (m_thumbProgress == 360)
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
            setMinimumHeight(8);
            setMaximumHeight(8);
            calculateBarParams();
            break;
        case Ring:
            setMinimumSize(8, 8);
            setMaximumSize(64, 64);
            calculateRingParams();
            break;
    }
}

void ProgressIndicator::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen;

    auto drawBarBackground = [&]() {
        // Draw track inactive(background)
        pen.setColor(m_colorPalette.inactive);
        pen.setCapStyle(Qt::RoundCap);
        pen.setWidth(m_penWidth);
        painter.setPen(pen);
        painter.drawLine(m_trackStart, m_trackEnd);
    };

    auto drawBarNormalProgress = [&]() {
        // Calculate secondary progress value
        auto valueStartPos = m_padding;
        auto valueStartPoint = QPoint(valueStartPos, m_halfRectHeight);
        auto secondaryValuePos = int(m_actualLength * (m_secondaryValue - m_min) / (m_max - m_min)) + m_padding;
        auto secondaryValuePoint2 = QPoint(secondaryValuePos, m_halfRectHeight);

        // Draw secondary progress value
        pen.setColor(m_colorPalette.secondary);
        pen.setWidth(m_penWidth);
        painter.setPen(pen);
        painter.drawLine(valueStartPoint, secondaryValuePoint2);

        // Calculate progress value
        auto valueLength = int(m_actualLength * (m_value - m_min) / (m_max - m_min));
        auto valuePos = valueLength + m_padding;
        auto valuePoint = QPoint(valuePos, m_halfRectHeight);

        // Draw progress value
        pen.setColor(m_colorPalette.total);
        pen.setWidth(m_penWidth);
        painter.setPen(pen);
        painter.drawLine(valueStartPoint, valuePoint);

        // Calculate current task progress value
        auto curTaskValuePos = int(valueLength * (m_currentTaskValue - m_min) / (m_max - m_min)) + m_padding;
        auto curTaskValuePoint = QPoint(curTaskValuePos, m_halfRectHeight);

        // Draw current task progress value
        pen.setColor(m_colorPalette.currentTask);
        pen.setWidth(m_penWidth);
        painter.setPen(pen);
        painter.drawLine(valueStartPoint, curTaskValuePoint);
    };

    auto drawBarIndeterminateProgress = [&](){
        // Calculate progress value
        auto thumbLength = rect().width() / 3;
        auto thumbActualRight = qRound(m_thumbProgress * (m_actualLength + thumbLength) / 360.0) + m_padding;
        auto thumbActualLeft = thumbActualRight - thumbLength;
        QPoint point1;
        if (thumbActualLeft < m_padding)
            point1 = QPoint(m_padding, m_halfRectHeight);
        else
            point1 = QPoint(thumbActualLeft, m_halfRectHeight);

        QPoint point2;
        auto trackActualRight = m_trackEnd.x();
        if (thumbActualRight < m_padding)
            point2 = QPoint(m_padding, m_halfRectHeight);
        else if (thumbActualRight < trackActualRight)
            point2 = QPoint(thumbActualRight, m_halfRectHeight);
        else
            point2 = QPoint(trackActualRight, m_halfRectHeight);

        // Draw progress value
        pen.setColor(m_colorPalette.total);
        pen.setWidth(m_penWidth);
        painter.setPen(pen);
//        qDebug() << point1 << point2;
        painter.drawLine(point1, point2);

//        if (m_indeterminateThumbX - thumbLength > trackActualRight)
//            m_indeterminateThumbX = 0; // reset thumb pos
    };

    auto drawRingBackground = [&]() {
        pen.setColor(m_colorPalette.inactive);
        pen.setWidth(m_penWidth);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        painter.drawEllipse(m_ringRect);
    };

    auto drawRingNormalProgress = [&]() {
        auto start = []() { return 90 * 16; };
        auto getSpanAngle = [](int spanAngle) {
            return -spanAngle * 16;
        };

        // Calculate secondary progress value
        auto secondaryAngle = int(360 * (m_secondaryValue - m_min) / (m_max - m_min));
        auto secondarySpan = getSpanAngle(secondaryAngle);

        // Draw secondary progress value
        pen.setColor(m_colorPalette.secondary);
        painter.setPen(pen);
        painter.drawArc(m_ringRect, start(), secondarySpan);

        // Calculate progress value
        auto valueAngle = int(360 * (m_value - m_min) / (m_max - m_min));
        auto valueSpan = getSpanAngle(valueAngle);

        // Draw secondary progress value
        pen.setColor(m_colorPalette.total);
        painter.setPen(pen);
        painter.drawArc(m_ringRect, start(), valueSpan);

        // Calculate current task progress value
        auto curTaskAngle = int(valueAngle * (m_currentTaskValue - m_min) / (m_max - m_min));
        auto curTaskSpan = getSpanAngle(curTaskAngle);

        // Draw secondary progress value
        pen.setColor(m_colorPalette.currentTask);
        painter.setPen(pen);
        painter.drawArc(m_ringRect, start(), curTaskSpan);
    };

    auto drawRingIndeterminateProgress = [&]() {
        int startAngle = -m_thumbProgress * 16;
        int spanAngle = 120 * 16;
        pen.setColor(m_colorPalette.total);
        painter.setPen(pen);
        painter.drawArc(m_ringRect, startAngle, spanAngle);
    };

    if (m_indicatorStyle == HorizontalBar) {
        drawBarBackground();
        if (!m_indeterminate)
            drawBarNormalProgress();
        else
            drawBarIndeterminateProgress();
    } else if (m_indicatorStyle == Ring) {
        drawRingBackground();
        if (!m_indeterminate)
            drawRingNormalProgress();
        else
            drawRingIndeterminateProgress();
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
void ProgressIndicator::resizeEvent(QResizeEvent *event) {
    switch (m_indicatorStyle) {
        case HorizontalBar:
            calculateBarParams();
            break;
        case Ring:
            calculateRingParams();
            break;
    }
}

void ProgressIndicator::calculateBarParams() {
    m_penWidth = rect().height();
    m_padding = m_penWidth / 2;
    m_halfRectHeight = rect().height() / 2;

    // Calculate track inactive(background)
    m_trackStart = QPoint(rect().left() + m_padding, m_halfRectHeight);
    m_trackEnd = QPoint(rect().right() - m_padding, m_halfRectHeight);
    m_actualLength = rect().width() - m_padding - m_padding;
}

void ProgressIndicator::calculateRingParams() {
    auto minLength = qMin(rect().width(), rect().height());
    m_penWidth = int(minLength * 0.15);
    m_padding = m_penWidth;

    auto left = (rect().width() - minLength) / 2 + m_padding;
    auto top = (rect().height() - minLength) / 2 + m_padding;
    auto width = minLength - 2 * m_padding;
    auto height = width;
    m_ringRect = QRect(left, top, width, height);
}
