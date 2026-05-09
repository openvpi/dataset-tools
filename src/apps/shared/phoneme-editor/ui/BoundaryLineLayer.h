#pragma once

#include <QWidget>
#include <utility>

#include <dstools/ViewportController.h>
#include <dstools/TimePos.h>

#include "IBoundaryModel.h"

namespace dstools {
namespace phonemelabeler {

using dsfw::widgets::ViewportController;
using dsfw::widgets::ViewportState;

class BoundaryLineLayer : public QWidget {
    Q_OBJECT

public:
    explicit BoundaryLineLayer(QWidget *parent = nullptr);

    void setBoundaryModel(IBoundaryModel *model);
    void setCoordinateTransformer(const ViewportState &state, int pixelWidth);

    int hoveredBoundary() const { return m_hoveredBoundary; }

public slots:
    void setHoveredBoundary(int idx);
    void setDraggedBoundary(int idx);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    [[nodiscard]] double timeToX(double timeSec) const;

    IBoundaryModel *m_boundaryModel = nullptr;

    double m_viewStart = 0.0;
    double m_viewEnd = 10.0;
    int m_pixelWidth = 1200;

    int m_hoveredBoundary = -1;
    int m_draggedBoundary = -1;
};

} // namespace phonemelabeler
} // namespace dstools