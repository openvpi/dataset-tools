//
// Created by fluty on 2023/11/14.
//
#include <QDebug>
#include <QScrollBar>
#include <QWheelEvent>

#ifdef Q_OS_WIN
#    include <Windows.h>
#endif

#include "TracksGraphicsView.h"
TracksGraphicsView::TracksGraphicsView(QWidget *parent) {
    setRenderHint(QPainter::Antialiasing);
    setAttribute(Qt::WA_AcceptTouchEvents);
    // setCacheMode(QGraphicsView::CacheNone);
    // setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    m_scaleXAnimation.setTargetObject(this);
    m_scaleXAnimation.setPropertyName("scaleX");
    m_scaleXAnimation.setDuration(150);
    m_scaleXAnimation.setEasingCurve(QEasingCurve::OutCubic);

    m_scaleYAnimation.setTargetObject(this);
    m_scaleYAnimation.setPropertyName("scaleY");
    m_scaleYAnimation.setDuration(150);
    m_scaleYAnimation.setEasingCurve(QEasingCurve::OutCubic);

    m_horizontalScrollBarAnimation.setTargetObject(this);
    m_horizontalScrollBarAnimation.setPropertyName("horizontalScrollBarValue");
    m_horizontalScrollBarAnimation.setDuration(150);
    m_horizontalScrollBarAnimation.setEasingCurve(QEasingCurve::OutCubic);

    m_verticalScrollBarAnimation.setTargetObject(this);
    m_verticalScrollBarAnimation.setPropertyName("verticalScrollBarValue");
    m_verticalScrollBarAnimation.setDuration(150);
    m_verticalScrollBarAnimation.setEasingCurve(QEasingCurve::OutCubic);

    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &TracksGraphicsView::notifyVisibleRectChanged);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &TracksGraphicsView::notifyVisibleRectChanged);
}
TracksGraphicsView::~TracksGraphicsView() {
}
qreal TracksGraphicsView::scaleX() {
    return m_scaleX;
}
void TracksGraphicsView::setScaleX(const qreal sx) {
    m_scaleX = sx;
    emit scaleChanged(m_scaleX, m_scaleY);
}
qreal TracksGraphicsView::scaleY() {
    return m_scaleY;
}
void TracksGraphicsView::setScaleY(const qreal sy) {
    m_scaleY = sy;
    emit scaleChanged(m_scaleX, m_scaleY);
}
int TracksGraphicsView::horizontalScrollBarValue() {
    return horizontalScrollBar()->value();
}
void TracksGraphicsView::setHorizontalScrollBarValue(int value) {
    horizontalScrollBar()->setValue(value);
}
int TracksGraphicsView::verticalScrollBarValue() {
    return verticalScrollBar()->value();
}
void TracksGraphicsView::setVerticalScrollBarValue(int value) {
    verticalScrollBar()->setValue(value);
}
QRectF TracksGraphicsView::visibleRect() const {
    auto viewportRect = viewport()->rect();
    auto leftTop = mapToScene(viewportRect.left(), viewportRect.top());
    auto rightBottom = mapToScene(viewportRect.width(), viewportRect.height());
    return QRectF(leftTop, rightBottom);
}
bool TracksGraphicsView::event(QEvent *event) {
#ifdef Q_OS_MAC
    // Mac Trackpad smooth zooming
    if (event->type() == QEvent::NativeGesture) {
        auto gestureEvent = static_cast<QNativeGestureEvent *>(event);

        if (gestureEvent->gestureType() == Qt::ZoomNativeGesture) {
            auto multiplier = gestureEvent->value() + 1;

            // Prevent negative zoom factors
            if (multiplier > 0) {
                m_scaleX *= multiplier;
                setScaleX(m_scaleX);
            }

            return true;
        }
    }
#endif
    return QGraphicsView::event(event);
}
void TracksGraphicsView::wheelEvent(QWheelEvent *event) {
    // auto cursorPosF = event->position();
    // auto cursorPos = QPoint(cursorPosF.x(), cursorPosF.y());
    // auto scenePos = mapToScene(cursorPos);
    //
    // auto viewWidth = viewport()->width();
    // auto viewHeight = viewport()->height();
    //
    // auto hScale = cursorPosF.x() / viewWidth;
    // auto vScale = cursorPosF.y() / viewHeight;

    auto deltaX = event->angleDelta().x();
    auto deltaY = event->angleDelta().y();

    if (event->modifiers() == Qt::ControlModifier) {
        auto targetScaleX = m_scaleX;
        if (deltaY > 0)
            targetScaleX = m_scaleX * (1 + 0.2 * deltaY / 120);
        else if (deltaY < 0)
            targetScaleX = m_scaleX / (1 + 0.2 * -deltaY / 120);

        if (targetScaleX > m_scaleXMax)
            targetScaleX = m_scaleXMax;

        if (qAbs(deltaY) < 120)
            setScaleX(targetScaleX);
        else {
            m_scaleXAnimation.stop();
            m_scaleXAnimation.setStartValue(m_scaleX);
            m_scaleXAnimation.setEndValue(targetScaleX);
            m_scaleXAnimation.start();
        }

        // auto viewPoint = transform().map((scenePos));
        // horizontalScrollBar()->setValue(qRound(viewPoint.x() - viewWidth * hScale));
        // verticalScrollBar()->setValue(qRound(viewPoint.y() - viewHeight * vScale));
    } else if (event->modifiers() == Qt::ShiftModifier) {
        auto targetScaleY = m_scaleY;
        if (deltaY > 0)
            targetScaleY = m_scaleY * (1 + 0.2 * deltaY / 120);
        else if (deltaY < 0)
            targetScaleY = m_scaleY / (1 + 0.2 * -deltaY / 120);

        if (targetScaleY < m_scaleYMin)
            targetScaleY = m_scaleYMin;
        else if (targetScaleY > m_scaleYMax)
            targetScaleY = m_scaleYMax;

        if (qAbs(deltaY) < 120)
            setScaleY(targetScaleY);
        else {
            m_scaleYAnimation.stop();
            m_scaleYAnimation.setStartValue(m_scaleY);
            m_scaleYAnimation.setEndValue(targetScaleY);
            m_scaleYAnimation.start();
        }

        // auto viewPoint = transform().map((scenePos));
        // horizontalScrollBar()->setValue(qRound(viewPoint.x() - viewWidth * hScale));
        // verticalScrollBar()->setValue(qRound(viewPoint.y() - viewHeight * vScale));
    } else if (event->modifiers() == Qt::AltModifier) {
        auto scrollLength = -1 * viewport()->width() * 0.2 * deltaX / 120;
        auto startValue = horizontalScrollBarValue();
        auto endValue = startValue + scrollLength;
        if (qAbs(deltaX) < 120)
            setHorizontalScrollBarValue(endValue);
        else {
            m_horizontalScrollBarAnimation.stop();
            m_horizontalScrollBarAnimation.setStartValue(startValue);
            m_horizontalScrollBarAnimation.setEndValue(endValue);
            m_horizontalScrollBarAnimation.start();
        }
    } else {
        auto scrollLength = -1 * viewport()->height() * 0.15 * deltaY / 120;
        auto startValue = verticalScrollBarValue();
        auto endValue = startValue + scrollLength;
        if (qAbs(deltaY) < 120)
            setVerticalScrollBarValue(endValue);
        else {
            m_verticalScrollBarAnimation.stop();
            m_verticalScrollBarAnimation.setStartValue(startValue);
            m_verticalScrollBarAnimation.setEndValue(endValue);
            m_verticalScrollBarAnimation.start();
        }
    }

    notifyVisibleRectChanged();
}
bool TracksGraphicsView::eventFilter(QObject *object, QEvent *event) {
    return QAbstractScrollArea::eventFilter(object, event);
}
void TracksGraphicsView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    notifyVisibleRectChanged();
}
void TracksGraphicsView::notifyVisibleRectChanged() {
    emit visibleRectChanged(visibleRect());
}
