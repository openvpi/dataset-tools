#include <algorithm>
#include <cmath>
#include <dsfw/widgets/ViewportController.h>

namespace dsfw::widgets {

    static constexpr double kZoomFactor = 1.5;

    static int roundToNearest10(int value) {
        if (value < 10) return value;
        return static_cast<int>(std::round(value / 10.0)) * 10;
    }

    ViewportController::ViewportController(QObject *parent) : QObject(parent) {
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
        if (m_sampleRate <= 0)
            return 0.0;
        return static_cast<double>(m_totalSamples) / m_sampleRate;
    }

    void ViewportController::setResolution(int resolution) {
        m_resolution = std::max(kMinResolution, roundToNearest10(resolution));
        syncStateFields();
        clampAndEmit();
    }

    void ViewportController::zoomIn(double centerSec) {
        if (!canZoomIn())
            return;

        int oldRes = m_resolution;
        m_resolution = std::max(kMinResolution,
                                roundToNearest10(static_cast<int>(std::round(m_resolution / kZoomFactor))));
        syncStateFields();

        double oldDuration = m_state.endSec - m_state.startSec;
        double ratio = (oldDuration > 0.0) ? (centerSec - m_state.startSec) / oldDuration : 0.5;
        double newDuration = oldDuration * static_cast<double>(m_resolution) / oldRes;
        m_state.startSec = centerSec - ratio * newDuration;
        m_state.endSec = m_state.startSec + newDuration;
        clampAndEmit();
    }

    void ViewportController::zoomOut(double centerSec) {
        int oldRes = m_resolution;
        m_resolution = roundToNearest10(static_cast<int>(std::round(m_resolution * kZoomFactor)));
        m_resolution = std::max(kMinResolution, m_resolution);
        syncStateFields();

        double oldDuration = m_state.endSec - m_state.startSec;
        double ratio = (oldDuration > 0.0) ? (centerSec - m_state.startSec) / oldDuration : 0.5;
        double newDuration = oldDuration * static_cast<double>(m_resolution) / oldRes;
        m_state.startSec = centerSec - ratio * newDuration;
        m_state.endSec = m_state.startSec + newDuration;
        clampAndEmit();
    }

    bool ViewportController::canZoomIn() const {
        return m_resolution > kMinResolution;
    }

    bool ViewportController::canZoomOut() const {
        return true;
    }

    void ViewportController::zoomAt(double centerSec, double factor) {
        if (factor <= 0.0)
            return;
        if (factor > 1.0) {
            zoomIn(centerSec);
        } else if (factor < 1.0) {
            zoomOut(centerSec);
        }
    }

    void ViewportController::setPixelsPerSecond(double pps) {
        if (m_sampleRate <= 0 || pps <= 0.0)
            return;
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
            if (m_state.startSec < 0.0)
                m_state.startSec = 0.0;
        }

        m_state.resolution = m_resolution;
        m_state.sampleRate = m_sampleRate;
        emit viewportChanged(m_state);
    }

} // namespace dsfw::widgets