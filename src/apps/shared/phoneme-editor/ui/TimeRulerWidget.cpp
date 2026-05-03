#include "TimeRulerWidget.h"

#include <QPainter>
#include <QPaintEvent>
#include <QWheelEvent>
#include <cmath>

namespace dstools {
namespace phonemelabeler {

TimeRulerWidget::TimeRulerWidget(ViewportController *viewport, QWidget *parent)
    : QWidget(parent),
    m_viewport(viewport)
{
    setFixedHeight(24);

    if (m_viewport) {
        connect(m_viewport, &ViewportController::viewportChanged,
                this, [this](const ViewportState &state) {
                    setViewport(state);
                });
        m_viewStart = m_viewport->state().startSec;
        m_viewEnd = m_viewport->state().endSec;
        m_pixelsPerSecond = m_viewport->state().pixelsPerSecond;
    }
}

void TimeRulerWidget::setViewport(const ViewportState &state) {
    m_viewStart = state.startSec;
    m_viewEnd = state.endSec;
    m_pixelsPerSecond = state.pixelsPerSecond;
    update();
}

int TimeRulerWidget::timeToX(double time) const {
    double viewDuration = m_viewEnd - m_viewStart;
    if (viewDuration <= 0.0 || width() <= 0) return 0;
    return static_cast<int>((time - m_viewStart) / viewDuration * width());
}

void TimeRulerWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(35, 35, 40));

    double viewDuration = m_viewEnd - m_viewStart;
    if (viewDuration <= 0.0) return;

    // Choose tick interval based on zoom level so ticks are ~60-120px apart
    double pixelsPerTick = 80.0;
    double secPerTick = pixelsPerTick / m_pixelsPerSecond;

    // Snap to nice intervals
    static const double niceSteps[] = {
        0.001, 0.002, 0.005,
        0.01, 0.02, 0.05,
        0.1, 0.2, 0.5,
        1.0, 2.0, 5.0,
        10.0, 20.0, 30.0, 60.0,
        120.0, 300.0, 600.0, 1800.0, 3600.0
    };
    double majorInterval = niceSteps[std::size(niceSteps) - 1];
    for (double step : niceSteps) {
        if (step >= secPerTick) {
            majorInterval = step;
            break;
        }
    }

    double minorInterval = majorInterval / 5.0;

    // Draw minor ticks
    painter.setPen(QPen(QColor(70, 70, 80), 1));
    double firstMinor = std::floor(m_viewStart / minorInterval) * minorInterval;
    for (double t = firstMinor; t <= m_viewEnd; t += minorInterval) {
        if (t < m_viewStart) continue;
        int x = timeToX(t);
        painter.drawLine(x, height() - 4, x, height());
    }

    // Draw major ticks + labels
    QFont font = painter.font();
    font.setPixelSize(10);
    painter.setFont(font);

    double firstMajor = std::floor(m_viewStart / majorInterval) * majorInterval;
    for (double t = firstMajor; t <= m_viewEnd; t += majorInterval) {
        if (t < m_viewStart) continue;
        int x = timeToX(t);

        // Major tick line
        painter.setPen(QPen(QColor(120, 120, 140), 1));
        painter.drawLine(x, height() - 10, x, height());

        // Label
        painter.setPen(QColor(180, 180, 200));
        QString label;
        if (majorInterval >= 1.0) {
            int totalSec = static_cast<int>(t);
            int min = totalSec / 60;
            int sec = totalSec % 60;
            double frac = t - std::floor(t);
            if (min > 0) {
                label = QString("%1:%2").arg(min).arg(sec, 2, 10, QChar('0'));
            } else if (frac > 0.0005) {
                label = QString::number(t, 'f', 1);
            } else {
                label = QString::number(totalSec) + "s";
            }
        } else if (majorInterval >= 0.01) {
            label = QString::number(t, 'f', 2) + "s";
        } else {
            label = QString::number(t, 'f', 3) + "s";
        }

        QRect textRect(x - 40, 0, 80, height() - 10);
        painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignBottom, label);
    }

    // Bottom line
    painter.setPen(QPen(QColor(80, 80, 100), 1));
    painter.drawLine(0, height() - 1, width(), height() - 1);
}

void TimeRulerWidget::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom: let parent handle or ignore
        QWidget::wheelEvent(event);
        return;
    }
    int d = (event->angleDelta().y() > 0) ? -1 : 1;
    emit entryScrollRequested(d);
    event->accept();
}

} // namespace phonemelabeler
} // namespace dstools
