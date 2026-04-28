#pragma once

#include <QWidget>
#include "ViewportController.h"

class QPainter;

namespace dstools {
namespace phonemelabeler {

/// Horizontal time ruler displaying second ticks, aligned with the viewport.
class TimeRulerWidget : public QWidget {
    Q_OBJECT

public:
    explicit TimeRulerWidget(ViewportController *viewport, QWidget *parent = nullptr);

    void setViewport(const ViewportState &state);

signals:
    void entryScrollRequested(int delta);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    [[nodiscard]] int timeToX(double time) const;

    ViewportController *m_viewport = nullptr;
    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;
    double m_pixelsPerSecond = 200.0;
};

} // namespace phonemelabeler
} // namespace dstools
