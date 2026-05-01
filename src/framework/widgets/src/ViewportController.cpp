#include <dsfw/widgets/ViewportController.h>
#include <algorithm>

namespace dsfw::widgets {

ViewportController::ViewportController(QObject *parent) : QObject(parent) {}

void ViewportController::setViewRange(double startSec, double endSec) {
    m_state.startSec = startSec;
    m_state.endSec = endSec;
    clampAndEmit();
}

void ViewportController::setPixelsPerSecond(double pps) {
    double oldPPS = m_state.pixelsPerSecond;
    m_state.pixelsPerSecond = std::clamp(pps, m_minPixelsPerSecond, m_maxPixelsPerSecond);
    double center = (m_state.startSec + m_state.endSec) / 2.0;
    double halfDuration = (m_state.endSec - m_state.startSec) / 2.0;
    double newHalfDuration = halfDuration * oldPPS / m_state.pixelsPerSecond;
    m_state.startSec = center - newHalfDuration;
    m_state.endSec = center + newHalfDuration;
    clampAndEmit();
}

void ViewportController::zoomAt(double centerSec, double factor) {
    if (factor == 0.0)
        return;
    double newPPS = m_state.pixelsPerSecond * factor;
    newPPS = std::clamp(newPPS, m_minPixelsPerSecond, m_maxPixelsPerSecond);
    double duration = m_state.endSec - m_state.startSec;
    if (duration == 0.0)
        return;
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

} // namespace dsfw::widgets
