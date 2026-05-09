#include "PlayCursorOverlay.h"

#include <QPainter>
#include <QPaintEvent>

#include <algorithm>

namespace dstools {

PlayCursorOverlay::PlayCursorOverlay(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setStyleSheet("background: transparent;");
}

void PlayCursorOverlay::setPlayheadPosition(double positionSec) {
    if (m_positionSec == positionSec) return;
    m_positionSec = positionSec;
    update();
}

void PlayCursorOverlay::setViewStart(double startSec) {
    m_viewStart = startSec;
}

void PlayCursorOverlay::setViewEnd(double endSec) {
    m_viewEnd = endSec;
}

void PlayCursorOverlay::setPixelWidth(int pixelWidth) {
    m_pixelWidth = pixelWidth;
}

double PlayCursorOverlay::timeToX(double timeSec) const {
    double dur = m_viewEnd - m_viewStart;
    if (dur <= 0.0 || m_pixelWidth <= 0) return 0.0;
    return (timeSec - m_viewStart) / dur * m_pixelWidth;
}

void PlayCursorOverlay::paintEvent(QPaintEvent *) {
    if (m_positionSec < 0.0) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    double x = std::clamp(timeToX(m_positionSec), 0.0, static_cast<double>(width()));

    painter.setPen(QPen(m_color, m_lineWidth));
    painter.drawLine(QPointF(x, 0), QPointF(x, height()));
}

} // namespace dstools