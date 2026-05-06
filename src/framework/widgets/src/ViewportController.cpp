#include <dsfw/widgets/ViewportController.h>
#include <algorithm>
#include <cmath>

namespace dsfw::widgets {

// Discrete resolution steps (samples per pixel).
// All values are round numbers matching common zoom levels.
// Zoom always snaps to the nearest entry — no continuous scaling.
static const std::vector<int> kResolutionTable = {
    10, 20, 30, 40, 60, 80, 100, 150, 200, 300, 400
};

const std::vector<int> &ViewportController::resolutionTable() {
    return kResolutionTable;
}

int ViewportController::findResolutionIndex(int res) const {
    const auto &table = resolutionTable();
    // Find closest entry
    int bestIdx = 0;
    int bestDist = std::abs(table[0] - res);
    for (int i = 1; i < static_cast<int>(table.size()); ++i) {
        int dist = std::abs(table[i] - res);
        if (dist < bestDist) {
            bestDist = dist;
            bestIdx = i;
        }
    }
    return bestIdx;
}

ViewportController::ViewportController(QObject *parent) : QObject(parent) {
    // Default resolution = 40 (index 4 in table)
    m_resolutionIndex = findResolutionIndex(m_resolution);
    m_resolution = resolutionTable()[m_resolutionIndex];
    updatePPS();
}

void ViewportController::setAudioParams(int sampleRate, int64_t totalSamples) {
    m_sampleRate = sampleRate;
    m_totalSamples = totalSamples;
    updatePPS();
    clampAndEmit();
}

void ViewportController::setTotalDuration(double duration) {
    m_totalSamples = static_cast<int64_t>(duration * m_sampleRate);
    clampAndEmit();
}

double ViewportController::totalDuration() const {
    if (m_sampleRate <= 0) return 0.0;
    return static_cast<double>(m_totalSamples) / m_sampleRate;
}

void ViewportController::setResolution(int resolution) {
    m_resolutionIndex = findResolutionIndex(resolution);
    m_resolution = resolutionTable()[m_resolutionIndex];
    updatePPS();
    // Adjust view range to maintain center
    double center = viewCenter();
    double halfDur = duration() / 2.0;
    // Recalculate based on new PPS — keep center stable
    m_state.startSec = center - halfDur;
    m_state.endSec = center + halfDur;
    clampAndEmit();
}

void ViewportController::zoomIn(double centerSec) {
    if (!canZoomIn()) return;
    double oldDuration = duration();
    double ratio = (oldDuration > 0.0) ? (centerSec - m_state.startSec) / oldDuration : 0.5;

    m_resolutionIndex--;
    m_resolution = resolutionTable()[m_resolutionIndex];
    updatePPS();

    // Zoom in → resolution decreases → more PPS → visible duration shrinks
    // newDuration = oldDuration * newRes / oldRes  = smaller when newRes < oldRes
    double newDuration = oldDuration * static_cast<double>(m_resolution) / resolutionTable()[m_resolutionIndex + 1];
    m_state.startSec = centerSec - ratio * newDuration;
    m_state.endSec = m_state.startSec + newDuration;
    clampAndEmit();
}

void ViewportController::zoomOut(double centerSec) {
    if (!canZoomOut()) return;
    double oldDuration = duration();
    double ratio = (oldDuration > 0.0) ? (centerSec - m_state.startSec) / oldDuration : 0.5;

    m_resolutionIndex++;
    m_resolution = resolutionTable()[m_resolutionIndex];
    updatePPS();

    // Zoom out → resolution increases → fewer PPS → visible duration grows
    double newDuration = oldDuration * static_cast<double>(m_resolution) / resolutionTable()[m_resolutionIndex - 1];
    m_state.startSec = centerSec - ratio * newDuration;
    m_state.endSec = m_state.startSec + newDuration;
    clampAndEmit();
}

bool ViewportController::canZoomIn() const {
    return m_resolutionIndex > 0;
}

bool ViewportController::canZoomOut() const {
    return m_resolutionIndex < static_cast<int>(resolutionTable().size()) - 1;
}

void ViewportController::setPixelsPerSecond(double pps) {
    if (m_sampleRate <= 0) return;
    int newRes = static_cast<int>(std::round(static_cast<double>(m_sampleRate) / pps));
    setResolution(newRes);
}

void ViewportController::zoomAt(double centerSec, double factor) {
    if (factor <= 0.0) return;
    if (factor > 1.0) {
        zoomIn(centerSec);
    } else if (factor < 1.0) {
        zoomOut(centerSec);
    }
}

void ViewportController::setViewRange(double startSec, double endSec) {
    m_state.startSec = startSec;
    m_state.endSec = endSec;
    clampAndEmit();
}

void ViewportController::scrollBy(double deltaSec) {
    m_state.startSec += deltaSec;
    m_state.endSec += deltaSec;
    clampAndEmit();
}

void ViewportController::updatePPS() {
    if (m_sampleRate > 0 && m_resolution > 0) {
        m_state.pixelsPerSecond = static_cast<double>(m_sampleRate) / m_resolution;
    }
}

void ViewportController::clampAndEmit() {
    double totalDur = totalDuration();

    if (m_state.startSec < 0.0) {
        double diff = -m_state.startSec;
        m_state.startSec = 0.0;
        m_state.endSec += diff;
    }
    if (totalDur > 0.0 && m_state.endSec > totalDur) {
        double diff = m_state.endSec - totalDur;
        m_state.endSec = totalDur;
        m_state.startSec -= diff;
        if (m_state.startSec < 0.0) m_state.startSec = 0.0;
    }
    emit viewportChanged(m_state);
}

} // namespace dsfw::widgets
