#pragma once

#include <QWidget>
#include <vector>

#include <dstools/ViewportController.h>

namespace dstools {

using dstools::widgets::ViewportController;
using dstools::widgets::ViewportState;

class SliceNumberLayer : public QWidget {
    Q_OBJECT

public:
    explicit SliceNumberLayer(ViewportController *viewport, QWidget *parent = nullptr);
    ~SliceNumberLayer() override = default;

    void setSlicePoints(const std::vector<double> &points);

    void setViewport(const ViewportState &state);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    [[nodiscard]] int timeToX(double time) const;

    ViewportController *m_viewport = nullptr;
    std::vector<double> m_slicePoints;
    double m_totalDuration = 0.0;
    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;
};

} // namespace dstools
