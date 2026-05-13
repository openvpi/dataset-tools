#include "ChartPanelBase.h"
#include "IBoundaryModel.h"
#include "BoundaryDragController.h"

#include <dsfw/Theme.h>

#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QUndoStack>
#include <QWheelEvent>

#include <algorithm>
#include <limits>

namespace dstools {
namespace phonemelabeler {

ChartPanelBase::ChartPanelBase(const QString &id,
                               ViewportController *viewport,
                               QWidget *parent)
    : QWidget(parent)
    , m_id(id)
    , m_viewport(viewport)
{
    setMouseTracking(true);
}

void ChartPanelBase::onViewportUpdate(const CoordinateTransformer &xf) {
    if (m_xf.viewStart == xf.viewStart && m_xf.viewEnd == xf.viewEnd
        && m_xf.resolution == xf.resolution)
        return;
    m_xf = xf;
    m_cacheDirty = true;
    update();
}

void ChartPanelBase::setViewport(const ViewportState &state) {
    CoordinateTransformer xf;
    xf.updateFromState(state, width());
    onViewportUpdate(xf);
}

void ChartPanelBase::onPlayheadUpdate(const PlayheadState &state) {
    Q_UNUSED(state)
}

void ChartPanelBase::onActiveTierChanged(int tierIndex) {
    Q_UNUSED(tierIndex)
    update();
}

std::pair<int, int> ChartPanelBase::hitTestBoundary(int x) const {
    if (!m_boundaryModel)
        return {-1, -1};

    double clickTime = m_xf.xToTime(x);
    double halfWidthTime = m_xf.xToTime(x + kBoundaryHitWidth / 2)
                         - m_xf.xToTime(x - kBoundaryHitWidth / 2);
    double pixelRange = std::abs(halfWidthTime);

    int bestTier = -1, bestIdx = -1;
    double bestDist = std::numeric_limits<double>::max();

    int tierCount = m_boundaryModel->tierCount();
    for (int t = 0; t < tierCount; ++t) {
        int count = m_boundaryModel->boundaryCount(t);
        for (int b = 0; b < count; ++b) {
            double bTime = usToSec(m_boundaryModel->boundaryTime(t, b));
            double dist = std::abs(bTime - clickTime);
            if (dist < pixelRange / 2.0) {
                if (dist < bestDist || (dist == bestDist && (t > bestTier || b > bestIdx))) {
                    bestDist = dist;
                    bestTier = t;
                    bestIdx = b;
                }
            }
        }
    }

    return {bestTier, bestIdx};
}

std::pair<double, double> ChartPanelBase::findSurrounding(double timeSec) const {
    double outStart = m_xf.viewStart;
    double outEnd = m_xf.viewEnd;

    if (!m_boundaryModel)
        return {outStart, outEnd};

    int activeTier = m_boundaryModel->activeTierIndex();
    if (activeTier < 0 || activeTier >= m_boundaryModel->tierCount())
        return {outStart, outEnd};

    int count = m_boundaryModel->boundaryCount(activeTier);
    for (int b = 0; b < count; ++b) {
        double t = usToSec(m_boundaryModel->boundaryTime(activeTier, b));
        if (t <= timeSec)
            outStart = t;
        if (t > timeSec) {
            outEnd = t;
            break;
        }
    }

    return {outStart, outEnd};
}

void ChartPanelBase::paintEvent(QPaintEvent *) {
    if (m_cacheDirty) {
        rebuildCache();
        m_cacheDirty = false;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    drawChartImage(painter);
    drawContent(painter);

    if (m_showBoundaryOverlay && m_boundaryModel)
        drawBoundaries(painter);
}

void ChartPanelBase::resizeEvent(QResizeEvent *event) {
    m_xf.pixelWidth = width();
    m_cacheDirty = true;
    QWidget::resizeEvent(event);
}

void ChartPanelBase::drawBoundaries(QPainter &painter) {
    int activeTier = m_boundaryModel->activeTierIndex();
    if (activeTier < 0) return;

    const auto &pal = dsfw::Theme::instance().palette().phonemeEditor;

    int count = m_boundaryModel->boundaryCount(activeTier);
    for (int i = 0; i < count; ++i) {
        double time = usToSec(m_boundaryModel->boundaryTime(activeTier, i));
        if (time < m_xf.viewStart || time > m_xf.viewEnd)
            continue;
        int x = static_cast<int>(m_xf.timeToX(time));

        QColor color = pal.boundaryNormal;
        if (m_hoveredBoundary.first == activeTier && m_hoveredBoundary.second == i)
            color = pal.boundaryHovered;

        painter.setPen(QPen(color, 2));
        painter.drawLine(x, 0, x, height());
    }
}

void ChartPanelBase::mousePressEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    auto [tier, idx] = hitTestBoundary(event->pos().x());
    if (tier >= 0 && idx >= 0 && m_dragController && m_boundaryModel) {
        m_dragController->startDrag(tier, idx, m_boundaryModel);
        setCursor(Qt::SizeHorCursor);
        event->accept();
        return;
    }

    emit positionClicked(m_xf.xToTime(event->pos().x()));

    m_viewportDragging = true;
    m_dragStartPos = event->pos();
    m_dragStartXfStart = m_xf.viewStart;
    setCursor(Qt::ClosedHandCursor);
    onViewportDragStart(m_xf.xToTime(event->pos().x()));
    event->accept();
}

void ChartPanelBase::mouseMoveEvent(QMouseEvent *event) {
    if (m_dragController && m_dragController->isDragging()) {
        TimePos currentTime = secToUs(m_xf.xToTime(event->pos().x()));
        m_dragController->updateDrag(currentTime);
        emit boundaryDragging();
        return;
    }

    if (m_viewportDragging && m_viewport) {
        double deltaSec = m_xf.xToTime(event->pos().x())
                        - m_xf.xToTime(m_dragStartPos.x());
        m_viewport->scrollBy(-deltaSec);
        return;
    }

    auto [tier, idx] = hitTestBoundary(event->pos().x());
    int activeTier = m_boundaryModel ? m_boundaryModel->activeTierIndex() : -1;

    if (tier >= 0) {
        if (m_hoveredBoundary.first != tier || m_hoveredBoundary.second != idx) {
            m_hoveredBoundary = {tier, idx};
            setCursor(Qt::SizeHorCursor);
            emit hoveredBoundaryChanged(tier == activeTier ? idx : -1);
            update();
        }
    } else {
        if (m_hoveredBoundary.first >= 0) {
            m_hoveredBoundary = {-1, -1};
            setCursor(Qt::ArrowCursor);
            emit hoveredBoundaryChanged(-1);
            update();
        }
    }
}

void ChartPanelBase::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton) return;

    if (m_dragController && m_dragController->isDragging()) {
        int draggedTier = m_dragController->draggedTier();
        int draggedBoundary = m_dragController->draggedBoundary();
        TimePos finalTime = secToUs(m_xf.xToTime(event->pos().x()));
        m_dragController->endDrag(finalTime, m_undoStack);
        unsetCursor();
        emit boundaryDragFinished(draggedTier, draggedBoundary, finalTime);
    }

    m_viewportDragging = false;
    setCursor(hitTestBoundary(event->pos().x()).first >= 0
              ? Qt::SizeHorCursor : Qt::ArrowCursor);
}

void ChartPanelBase::contextMenuEvent(QContextMenuEvent *event) {
    if (!m_playWidget || !m_boundaryModel) {
        event->accept();
        return;
    }

    double clickTime = m_xf.xToTime(event->pos().x());
    auto [segStart, segEnd] = findSurrounding(clickTime);
    playSegmentBetween(segStart, segEnd);

    event->accept();
}

void ChartPanelBase::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        event->ignore();
        return;
    }
    if (event->modifiers() & Qt::ShiftModifier) {
        double delta = event->angleDelta().y() / 120.0;
        double factor = (delta > 0) ? 1.25 : 0.8;
        onVerticalZoom(factor);
        event->accept();
    } else {
        if (m_viewport) {
            double scrollSec = (event->angleDelta().y() > 0) ? -0.5 : 0.5;
            m_viewport->scrollBy(scrollSec);
        }
        event->accept();
    }
}

void ChartPanelBase::playSegmentBetween(double startSec, double endSec) {
    if (!m_playWidget) return;

    m_playWidget->setPlayRange(startSec, endSec);
    m_playWidget->seek(startSec);
    m_playWidget->setPlaying(true);
}

} // namespace phonemelabeler
} // namespace dstools