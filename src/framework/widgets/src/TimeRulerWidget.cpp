#include <dsfw/widgets/TimeRulerWidget.h>

#include <QPainter>
#include <QPaintEvent>
#include <QWheelEvent>
#include <cmath>
#include <algorithm>

namespace dsfw::widgets {

static const TimeRulerWidget::TimescaleLevel kLevels[] = {
    {0.01,   0.005},  // 10ms / 5ms
    {0.1,    0.01},   // 100ms / 10ms
    {1.0,    0.1},    // 1s / 100ms
    {10.0,   1.0},    // 10s / 1s
    {60.0,   10.0},   // 1min / 10s
    {600.0,  60.0},   // 10min / 1min
    {3600.0, 600.0},  // 1h / 10min
};

static constexpr int kLevelCount = static_cast<int>(std::size(kLevels));

static double smoothStep(double edge0, double edge1, double x) {
    double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

TimeRulerWidget::TimeRulerWidget(ViewportController *viewport, QWidget *parent)
    : QWidget(parent), m_viewport(viewport) {
    setFixedHeight(24);

    if (m_viewport) {
        connect(m_viewport, &ViewportController::viewportChanged,
                this, [this](const ViewportState &state) { setViewport(state); });
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

TimeRulerWidget::TimescaleLevel TimeRulerWidget::findLevel(double pps) {
    for (int i = 0; i < kLevelCount; ++i) {
        if (kLevels[i].minorSec * pps >= kMinMinorStepPx)
            return kLevels[i];
    }
    return kLevels[kLevelCount - 1];
}

QString TimeRulerWidget::formatTime(double timeSec, double intervalSec) {
    if (intervalSec >= 3600.0) {
        int totalSec = static_cast<int>(timeSec);
        int hr = totalSec / 3600;
        int min = (totalSec % 3600) / 60;
        int sec = totalSec % 60;
        return QStringLiteral("%1:%2:%3")
            .arg(hr)
            .arg(min, 2, 10, QChar('0'))
            .arg(sec, 2, 10, QChar('0'));
    }
    if (intervalSec >= 60.0) {
        int totalSec = static_cast<int>(timeSec);
        int min = totalSec / 60;
        int sec = totalSec % 60;
        return QStringLiteral("%1:%2")
            .arg(min)
            .arg(sec, 2, 10, QChar('0'));
    }
    if (intervalSec >= 1.0) {
        return QString::number(static_cast<int>(timeSec)) + QStringLiteral("s");
    }
    if (intervalSec >= 0.1) {
        return QString::number(timeSec, 'f', 1) + QStringLiteral("s");
    }
    if (intervalSec >= 0.01) {
        return QString::number(timeSec, 'f', 2) + QStringLiteral("s");
    }
    return QString::number(timeSec * 1000.0, 'f', 1) + QStringLiteral("ms");
}

void TimeRulerWidget::paintEvent(QPaintEvent * /*event*/) {
    QPainter painter(this);
    painter.fillRect(rect(), QColor(35, 35, 40));

    double viewDuration = m_viewEnd - m_viewStart;
    if (viewDuration <= 0.0) return;

    auto level = findLevel(m_pixelsPerSecond);

    QFont font = painter.font();
    font.setPixelSize(10);
    painter.setFont(font);

    int h = height();
    static constexpr int kMajorTickH = 12;
    static constexpr int kMinorTickH = 6;

    double actualMinorSpacing = level.minorSec * m_pixelsPerSecond;
    double minorAlpha = smoothStep(kMinMinorStepPx, kMinMinorStepPx * 2.0, actualMinorSpacing);
    double actualMajorSpacing = level.majorSec * m_pixelsPerSecond;
    double majorAlpha = smoothStep(kMinMinorStepPx, kMinMinorStepPx * 2.0, actualMajorSpacing);

    // Draw minor ticks with fade
    if (minorAlpha > 0.01) {
        QColor minorColor(80, 80, 100);
        minorColor.setAlphaF(minorAlpha);
        QPen minorPen(minorColor, 1);
        painter.setPen(minorPen);

        double firstTick = std::floor(m_viewStart / level.minorSec) * level.minorSec;
        for (double t = firstTick; t <= m_viewEnd; t += level.minorSec) {
            if (t < m_viewStart) continue;
            int x = timeToX(t);
            painter.drawLine(x, h - kMinorTickH, x, h);
        }
    }

    // Draw major ticks + labels with fade
    if (majorAlpha > 0.01) {
        QColor majorLineColor(140, 140, 160);
        majorLineColor.setAlphaF(majorAlpha);
        QPen majorPen(majorLineColor, 1);
        painter.setPen(majorPen);

        QColor labelColor(180, 180, 200);
        labelColor.setAlphaF(majorAlpha);

        double firstMajor = std::floor(m_viewStart / level.majorSec) * level.majorSec;
        for (double t = firstMajor; t <= m_viewEnd; t += level.majorSec) {
            if (t < m_viewStart) continue;
            int x = timeToX(t);
            painter.drawLine(x, h - kMajorTickH, x, h);

            painter.setPen(labelColor);
            QString label = formatTime(t, level.majorSec);
            QRect textRect(x - 40, 0, 80, h - kMajorTickH);
            painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignBottom, label);
            painter.setPen(majorPen);
        }
    }

    // Bottom border
    painter.setPen(QPen(QColor(80, 80, 100), 1));
    painter.drawLine(0, h - 1, width(), h - 1);
}

void TimeRulerWidget::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        QWidget::wheelEvent(event);
        return;
    }
    int d = (event->angleDelta().y() > 0) ? -1 : 1;
    emit entryScrollRequested(d);
    event->accept();
}

} // namespace dsfw::widgets
