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

    m_hBarAnimation.setTargetObject(this);
    m_hBarAnimation.setPropertyName("horizontalScrollBarValue");
    m_hBarAnimation.setDuration(150);
    m_hBarAnimation.setEasingCurve(QEasingCurve::OutCubic);

    m_vBarAnimation.setTargetObject(this);
    m_vBarAnimation.setPropertyName("verticalScrollBarValue");
    m_vBarAnimation.setDuration(150);
    m_vBarAnimation.setEasingCurve(QEasingCurve::OutCubic);

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
int TracksGraphicsView::hBarValue() {
    return horizontalScrollBar()->value();
}
void TracksGraphicsView::setHBarValue(int value) {
    horizontalScrollBar()->setValue(value);
}
int TracksGraphicsView::vBarValue() {
    return verticalScrollBar()->value();
}
void TracksGraphicsView::setVBarValue(int value) {
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
    auto cursorPosF = event->position();
    auto cursorPos = QPoint(cursorPosF.x(), cursorPosF.y());
    auto scenePos = mapToScene(cursorPos);

    auto deltaX = event->angleDelta().x();
    auto deltaY = event->angleDelta().y();

    if (event->modifiers() == Qt::ControlModifier) {
        auto targetScaleX = m_scaleX;
        if (deltaY > 0)
            targetScaleX = m_scaleX * (1 + m_hZoomingStep * deltaY / 120);
        else if (deltaY < 0)
            targetScaleX = m_scaleX / (1 + m_hZoomingStep * -deltaY / 120);

        if (targetScaleX > m_scaleXMax)
            targetScaleX = m_scaleXMax;

        auto ratio = targetScaleX / m_scaleX;
        auto targetSceneX = scenePos.x() * ratio;
        auto targetValue = qRound(targetSceneX - cursorPos.x());
        if (qAbs(deltaY) < 120) {
            setScaleX(targetScaleX);
            setHBarValue(targetValue);
        } else {
            m_scaleXAnimation.stop();
            m_scaleXAnimation.setStartValue(m_scaleX);
            m_scaleXAnimation.setEndValue(targetScaleX);

            m_hBarAnimation.stop();
            m_hBarAnimation.setStartValue(hBarValue());
            m_hBarAnimation.setEndValue(targetValue);

            m_scaleXAnimation.start();
            m_hBarAnimation.start();
        }
    } else if (event->modifiers() == Qt::ShiftModifier) {
        auto targetScaleY = m_scaleY;
        if (deltaY > 0)
            targetScaleY = m_scaleY * (1 + m_vZoomingStep * deltaY / 120);
        else if (deltaY < 0)
            targetScaleY = m_scaleY / (1 + m_vZoomingStep * -deltaY / 120);

        if (targetScaleY < m_scaleYMin)
            targetScaleY = m_scaleYMin;
        else if (targetScaleY > m_scaleYMax)
            targetScaleY = m_scaleYMax;

        auto ratio = targetScaleY / m_scaleY;
        auto targetSceneY = scenePos.y() * ratio;
        auto targetValue = qRound(targetSceneY - cursorPos.y());
        if (qAbs(deltaY) < 120) {
            setScaleY(targetScaleY);
            setVBarValue(targetValue);
        } else {
            m_scaleYAnimation.stop();
            m_scaleYAnimation.setStartValue(m_scaleY);
            m_scaleYAnimation.setEndValue(targetScaleY);

            m_vBarAnimation.stop();
            m_vBarAnimation.setStartValue(vBarValue());
            m_vBarAnimation.setEndValue(targetValue);

            m_scaleYAnimation.start();
            m_vBarAnimation.start();
        }

    } else if (event->modifiers() == Qt::AltModifier) {
        auto scrollLength = -1 * viewport()->width() * 0.2 * deltaX / 120;
        auto startValue = hBarValue();
        auto endValue = startValue + scrollLength;
        if (qAbs(deltaX) < 120)
            setHBarValue(endValue);
        else {
            m_hBarAnimation.stop();
            m_hBarAnimation.setStartValue(startValue);
            m_hBarAnimation.setEndValue(endValue);
            m_hBarAnimation.start();
        }
    } else {
        auto scrollLength = -1 * viewport()->height() * 0.15 * deltaY / 120;
        auto startValue = vBarValue();
        auto endValue = startValue + scrollLength;
        if (qAbs(deltaY) < 120)
            setVBarValue(endValue);
        else {
            m_vBarAnimation.stop();
            m_vBarAnimation.setStartValue(startValue);
            m_vBarAnimation.setEndValue(endValue);
            m_vBarAnimation.start();
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
