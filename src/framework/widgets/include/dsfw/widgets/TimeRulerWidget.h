#pragma once

#include <dsfw/widgets/WidgetsGlobal.h>
#include <dsfw/widgets/ViewportController.h>
#include <QWidget>

class QPainter;

namespace dsfw::widgets {

class DSFW_WIDGETS_API TimeRulerWidget : public QWidget {
    Q_OBJECT

public:
    explicit TimeRulerWidget(ViewportController *viewport, QWidget *parent = nullptr);

    void setViewport(const ViewportState &state);

signals:
    void entryScrollRequested(int delta);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

public:
    struct TimescaleLevel {
        double majorSec;    ///< Major tick interval (seconds).
        double minorSec;    ///< Minor tick interval (seconds).
    };

private:
    [[nodiscard]] int timeToX(double time) const;

    /// Find appropriate timescale level for the current PPS.
    [[nodiscard]] static TimescaleLevel findLevel(double pps);

    /// Format time label based on the time value.
    [[nodiscard]] static QString formatTime(double timeSec, double intervalSec);

    ViewportController *m_viewport = nullptr;
    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;
    double m_pixelsPerSecond = 200.0;

    static constexpr double kMinMinorStepPx = 60.0;
};

} // namespace dsfw::widgets
