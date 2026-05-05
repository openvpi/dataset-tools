#include <dsfw/widgets/TimeRulerWidget.h>

#include <QPainter>
#include <QPaintEvent>
#include <QWheelEvent>
#include <cmath>

namespace dsfw::widgets {

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

double TimeRulerWidget::smoothStep(double edge0, double edge1, double x) {
    double t = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

double TimeRulerWidget::spacingVisibility(double spacing, double minimumSpacing) {
    if (spacing < minimumSpacing) return 0.0;
    if (spacing > kFadeInSpacing) return 1.0;
    return smoothStep(minimumSpacing, kFadeInSpacing, spacing);
}

void TimeRulerWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(35, 35, 40));

    double viewDuration = m_viewEnd - m_viewStart;
    if (viewDuration <= 0.0) return;

    static const TickLevel levels[] = {
        {3600.0,  12, true},
        { 600.0,  10, true},
        {  60.0,   8, true},
        {  10.0,   7, true},
        {   1.0,   6, true},
        {   0.1,   5, true},
        {  0.01,   4, false},
    };

    QFont font = painter.font();
    font.setPixelSize(10);
    painter.setFont(font);

    for (const auto &level : levels) {
        double spacing = level.interval * m_pixelsPerSecond;
        double alpha = spacingVisibility(spacing, kMinimumSpacing);
        if (alpha <= 0.0) continue;

        int tickAlpha = static_cast<int>(alpha * 255);
        int labelAlpha = static_cast<int>(alpha * 200);

        QPen tickPen(QColor(120, 120, 140, tickAlpha), 1);
        painter.setPen(tickPen);

        double firstTick = std::floor(m_viewStart / level.interval) * level.interval;
        for (double t = firstTick; t <= m_viewEnd; t += level.interval) {
            if (t < m_viewStart) continue;
            int x = timeToX(t);
            painter.drawLine(x, height() - level.height, x, height());
        }

        if (level.showLabel) {
            painter.setPen(QColor(180, 180, 200, labelAlpha));
            for (double t = firstTick; t <= m_viewEnd; t += level.interval) {
                if (t < m_viewStart) continue;
                int x = timeToX(t);

                QString label;
                if (level.interval >= 3600.0) {
                    int totalSec = static_cast<int>(t);
                    int hr = totalSec / 3600;
                    int min = (totalSec % 3600) / 60;
                    int sec = totalSec % 60;
                    label = QString("%1:%2:%3")
                                .arg(hr)
                                .arg(min, 2, 10, QChar('0'))
                                .arg(sec, 2, 10, QChar('0'));
                } else if (level.interval >= 60.0) {
                    int totalSec = static_cast<int>(t);
                    int min = totalSec / 60;
                    int sec = totalSec % 60;
                    label = QString("%1:%2")
                                .arg(min)
                                .arg(sec, 2, 10, QChar('0'));
                } else if (level.interval >= 1.0) {
                    int totalSec = static_cast<int>(t);
                    label = QString::number(totalSec) + QStringLiteral("s");
                } else if (level.interval >= 0.1) {
                    label = QString::number(t, 'f', 1) + QStringLiteral("s");
                } else {
                    label = QString::number(t * 1000.0, 'f', 0) + QStringLiteral("ms");
                }

                QRect textRect(x - 40, 0, 80, height() - level.height);
                painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignBottom, label);
            }
        }
    }

    painter.setPen(QPen(QColor(80, 80, 100), 1));
    painter.drawLine(0, height() - 1, width(), height() - 1);
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
