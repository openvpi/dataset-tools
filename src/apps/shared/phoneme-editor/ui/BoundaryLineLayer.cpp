#include "BoundaryLineLayer.h"

#include <QPainter>
#include <QPaintEvent>

#include <dsfw/Theme.h>

#include <cmath>

namespace dstools {
namespace phonemelabeler {

BoundaryLineLayer::BoundaryLineLayer(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAutoFillBackground(false);
}

void BoundaryLineLayer::setBoundaryModel(IBoundaryModel *model) {
    m_boundaryModel = model;
    update();
}

void BoundaryLineLayer::setCoordinateTransformer(const ViewportState &state, int pixelWidth) {
    m_viewStart = state.startSec;
    m_viewEnd = state.endSec;
    m_pixelWidth = pixelWidth;
    update();
}

void BoundaryLineLayer::setHoveredBoundary(int idx) {
    m_hoveredBoundary = idx;
    update();
}

void BoundaryLineLayer::setDraggedBoundary(int idx) {
    m_draggedBoundary = idx;
    update();
}

double BoundaryLineLayer::timeToX(double timeSec) const {
    double dur = m_viewEnd - m_viewStart;
    if (dur <= 0.0 || m_pixelWidth <= 0) return 0.0;
    return (timeSec - m_viewStart) / dur * m_pixelWidth;
}

void BoundaryLineLayer::paintEvent(QPaintEvent *) {
    if (!m_boundaryModel) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    int activeTier = m_boundaryModel->activeTierIndex();
    int tierCount = m_boundaryModel->tierCount();

    for (int t = 0; t < tierCount; ++t) {
        int count = m_boundaryModel->boundaryCount(t);
        for (int b = 0; b < count; ++b) {
            double tSec = usToSec(m_boundaryModel->boundaryTime(t, b));
            if (tSec < m_viewStart || tSec > m_viewEnd) continue;

            int x = static_cast<int>(timeToX(tSec));

            QColor color;
            const auto &pal = dsfw::Theme::instance().palette().phonemeEditor;
            if (t == activeTier) {
                if (m_draggedBoundary >= 0 && b == m_draggedBoundary)
                    color = pal.boundaryDragged;
                else if (b == m_hoveredBoundary)
                    color = pal.boundaryHovered;
                else
                    color = pal.boundaryNormal;
            } else {
                color = QColor(pal.boundaryNormal.red(), pal.boundaryNormal.green(),
                               pal.boundaryNormal.blue(), 80);
            }

            painter.setPen(QPen(color, 1));
            painter.drawLine(x, 0, x, height());
        }
    }
}

} // namespace phonemelabeler
} // namespace dstools