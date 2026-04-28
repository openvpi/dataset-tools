#pragma once

#include <QObject>
#include <QPoint>

namespace dstools {
namespace phonemelabeler {

struct ViewportState {
    double startSec = 0.0;
    double endSec = 10.0;
    double pixelsPerSecond = 200.0;
};

class ViewportController : public QObject {
    Q_OBJECT

public:
    explicit ViewportController(QObject *parent = nullptr);

    void setViewRange(double startSec, double endSec);
    void setPixelsPerSecond(double pps);

    void zoomAt(double centerSec, double factor);
    void scrollBy(double deltaSec);

    void setTotalDuration(double duration) { m_totalDuration = duration; }
    [[nodiscard]] double totalDuration() const { return m_totalDuration; }

    [[nodiscard]] const ViewportState &state() const { return m_state; }
    [[nodiscard]] double pixelsPerSecond() const { return m_state.pixelsPerSecond; }
    [[nodiscard]] double startSec() const { return m_state.startSec; }
    [[nodiscard]] double endSec() const { return m_state.endSec; }
    [[nodiscard]] double viewCenter() const {
        return (m_state.startSec + m_state.endSec) / 2.0;
    }
    [[nodiscard]] double duration() const { return m_state.endSec - m_state.startSec; }

signals:
    void viewportChanged(const ViewportState &state);

private:
    ViewportState m_state;
    double m_totalDuration = 0.0;
    double m_minPixelsPerSecond = 10.0;
    double m_maxPixelsPerSecond = 5000.0;

    void clampAndEmit();
};

} // namespace phonemelabeler
} // namespace dstools
