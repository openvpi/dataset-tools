/// @file SliceNumberLayer.h
/// @brief Numeric slice label widget showing segment numbers (001, 002, ...).

#pragma once

#include <QWidget>
#include <vector>

#include <dstools/ViewportController.h>

namespace dstools {

using dstools::widgets::ViewportController;
using dstools::widgets::ViewportState;

/// @brief Displays numbered slice labels between boundary lines.
///
/// Similar to a single-tier IntervalTierView but simplified: only shows
/// numeric labels (001, 002, 003...) without text editing capability.
/// Boundary lines are synchronized with the parent WaveformPanel viewport.
class SliceNumberLayer : public QWidget {
    Q_OBJECT

public:
    explicit SliceNumberLayer(ViewportController *viewport, QWidget *parent = nullptr);
    ~SliceNumberLayer() override = default;

    /// Set slice point positions (in seconds, sorted).
    void setSlicePoints(const std::vector<double> &points);

    /// Update viewport state.
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
