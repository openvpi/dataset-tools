#include <dsfw/widgets/ViewportController.h>
#include <algorithm>
#include <cmath>

namespace dsfw::widgets {

// Logarithmic resolution step table (samples per pixel).
// 22 entries covering fine editing (10 spx) to full overview (44100 spx = 1 px/s).
// All values are round numbers. Zoom steps through this table sequentially.
static const std::vector<int> kResolutionTable = {
    10, 20, 30, 50, 80, 100, 150, 200, 300, 500, 800, 1000,
    1500, 2000, 3000, 5000, 8000, 10000, 15000, 20000, 30000, 44100
};

const std::vector<int> &ViewportController::resolutionTable() {
    return kResolutionTable;
}

int ViewportController::findResolutionIndex(int res) const {
    const auto &table = resolutionTable();
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
    m_resolutionIndex = findResolutionIndex(m_resolution);
    m_resolution = resolutionTable()[m_resolutionIndex];
    syncStateFields();
}

void ViewportController::setAudioParams(int sampleRate, int64_t totalSamples) {
    m_sampleRate = sampleRate;
    m_totalSamples = totalSamples;
    syncStateFields();
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
    syncStateFields();
    clampAndEmit();
}

void ViewportController::zoomIn(double centerSec) {
    if (!canZoomIn()) return;

    int oldRes = m_resolution;
    m_resolutionIndex--;
    m_resolution = resolutionTable()[m_resolutionIndex];
    syncStateFields();

    // Keep the center time at the same pixel position
    double oldDuration = m_state.endSec - m_state.startSec;
    double ratio = (oldDuration > 0.0) ? (centerSec - m_state.startSec) / oldDuration : 0.5;
    double newDuration = oldDuration * static_cast<double>(m_resolution) / oldRes;
    m_state.startSec = centerSec - ratio * newDuration;
    m_state.endSec = m_state.startSec + newDuration;
    clampAndEmit();
}

void ViewportController::zoomOut(double centerSec) {
    if (canZoomOut()) {
        int oldRes = m_resolution;
        m_resolutionIndex++;
        m_resolution = resolutionTable()[m_resolutionIndex];
        syncStateFields();

        double oldDuration = m_state.endSec - m_state.startSec;
        double ratio = (oldDuration > 0.0) ? (centerSec - m_state.startSec) / oldDuration : 0.5;
        double newDuration = oldDuration * static_cast<double>(m_resolution) / oldRes;
        m_state.startSec = centerSec - ratio * newDuration;
        m_state.endSec = m_state.startSec + newDuration;
    }
    clampAndEmit();
}

bool ViewportController::canZoomIn() const {
    return m_resolutionIndex > 0;
}

bool ViewportController::canZoomOut() const {
    return m_resolutionIndex < static_cast<int>(resolutionTable().size()) - 1;
}

void ViewportController::zoomAt(double centerSec, double factor) {
    if (factor <= 0.0) return;
    if (factor > 1.0) {
        zoomIn(centerSec);
    } else if (factor < 1.0) {
        zoomOut(centerSec);
    }
}

void ViewportController::setPixelsPerSecond(double pps) {
    if (m_sampleRate <= 0 || pps <= 0.0) return;
    int newRes = static_cast<int>(std::round(static_cast<double>(m_sampleRate) / pps));
    setResolution(newRes);
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

void ViewportController::syncStateFields() {
    m_state.resolution = m_resolution;
    m_state.sampleRate = m_sampleRate;
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

    // When at max resolution but viewport still shorter than total duration,
    // expand to show the full audio.
    if (!canZoomOut() && totalDur > 0.0 &&
        (m_state.endSec - m_state.startSec) < totalDur) {
        m_state.startSec = 0.0;
        m_state.endSec = totalDur;
    }

    // Ensure state fields are in sync before emitting
    m_state.resolution = m_resolution;
    m_state.sampleRate = m_sampleRate;
    emit viewportChanged(m_state);
}

} // namespace dsfw::widgets
