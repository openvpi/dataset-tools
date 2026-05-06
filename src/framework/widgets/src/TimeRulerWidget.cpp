#include <dsfw/widgets/TimeRulerWidget.h>

#include <QPainter>
#include <QPaintEvent>
#include <QWheelEvent>
#include <cmath>
#include <algorithm>

namespace dsfw::widgets {

static const TimeRulerWidget::TimescaleLevel kLevels[] = {
    {0.001,  0.0005},   // 1ms / 0.5ms
    {0.005,  0.001},    // 5ms / 1ms
    {0.01,   0.005},    // 10ms / 5ms
    {0.05,   0.01},     // 50ms / 10ms
    {0.1,    0.05},     // 100ms / 50ms
    {0.5,    0.1},      // 500ms / 100ms
    {1.0,    0.5},      // 1s / 500ms
    {5.0,    1.0},      // 5s / 1s
    {15.0,   5.0},      // 15s / 5s
    {30.0,   10.0},     // 30s / 10s
    {60.0,   15.0},     // 1min / 15s
    {300.0,  60.0},     // 5min / 1min
    {900.0,  300.0},    // 15min / 5min
    {3600.0, 900.0},    // 1h / 15min
};

static constexpr int kLevelCount = static_cast<int>(std::size(kLevels));

TimeRulerWidget::TimeRulerWidget(ViewportController *viewport, QWidget *parent)
    : QWidget(parent), m_viewport(viewport) {
    setFixedHeight(24);

    if (m_viewport) {
        connect(m_viewport, &ViewportController::viewportChanged,
                this, [this](const ViewportState &state) { setViewport(state); });
        m_viewStart = m_viewport->state().startSec;
        m_viewEnd = m_viewport->state().endSec;
        m_resolution = m_viewport->state().resolution;
        m_sampleRate = m_viewport->state().sampleRate;
    }
}

void TimeRulerWidget::setViewport(const ViewportState &state) {
    m_viewStart = state.startSec;
    m_viewEnd = state.endSec;
    m_resolution = state.resolution;
    m_sampleRate = state.sampleRate;
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

    auto level = findLevel(m_sampleRate > 0 && m_resolution > 0
                           ? static_cast<double>(m_sampleRate) / m_resolution
                           : 200.0);

    QFont font = painter.font();
    font.setPixelSize(10);
    painter.setFont(font);

    int h = height();
    static constexpr int kMajorTickH = 12;
    static constexpr int kMinorTickH = 6;

    // Draw minor ticks (no fade — level is selected by findLevel to guarantee adequate spacing)
    {
        QPen minorPen(QColor(80, 80, 100), 1);
        painter.setPen(minorPen);

        double firstTick = std::floor(m_viewStart / level.minorSec) * level.minorSec;
        for (double t = firstTick; t <= m_viewEnd; t += level.minorSec) {
            if (t < m_viewStart) continue;
            int x = timeToX(t);
            painter.drawLine(x, h - kMinorTickH, x, h);
        }
    }

    // Draw major ticks + labels
    {
        QPen majorPen(QColor(140, 140, 160), 1);
        painter.setPen(majorPen);

        QColor labelColor(180, 180, 200);

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
