#include <dstools/ViewportController.h>
#include <algorithm>

namespace dstools::widgets {

ViewportController::ViewportController(QObject *parent) : QObject(parent) {}

void ViewportController::setViewRange(double startSec, double endSec) {
    m_state.startSec = startSec;
    m_state.endSec = endSec;
    clampAndEmit();
}

void ViewportController::setPixelsPerSecond(double pps) {
    m_state.pixelsPerSecond = std::clamp(pps, m_minPixelsPerSecond, m_maxPixelsPerSecond);
    emit viewportChanged(m_state);
}

void ViewportController::zoomAt(double centerSec, double factor) {
    double newPPS = m_state.pixelsPerSecond * factor;
    newPPS = std::clamp(newPPS, m_minPixelsPerSecond, m_maxPixelsPerSecond);
    double duration = m_state.endSec - m_state.startSec;
    double newDuration = duration / factor;
    double ratio = (centerSec - m_state.startSec) / duration;
    double newStart = centerSec - ratio * newDuration;
    double newEnd = newStart + newDuration;
    m_state.startSec = newStart;
    m_state.endSec = newEnd;
    m_state.pixelsPerSecond = newPPS;
    clampAndEmit();
}

void ViewportController::scrollBy(double deltaSec) {
    m_state.startSec += deltaSec;
    m_state.endSec += deltaSec;
    clampAndEmit();
}

void ViewportController::clampAndEmit() {
    if (m_state.startSec < 0.0) {
        double diff = -m_state.startSec;
        m_state.startSec = 0.0;
        m_state.endSec += diff;
    }
    if (m_totalDuration > 0.0 && m_state.endSec > m_totalDuration) {
        double diff = m_state.endSec - m_totalDuration;
        m_state.endSec = m_totalDuration;
        m_state.startSec -= diff;
        if (m_state.startSec < 0.0) m_state.startSec = 0.0;
    }
    emit viewportChanged(m_state);
}

} // namespace dstools::widgets
