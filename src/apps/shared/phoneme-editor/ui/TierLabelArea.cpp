#include "TierLabelArea.h"

#include "IBoundaryModel.h"

#include <dsfw/TimePos.h>

namespace dstools {


    TierLabelArea::TierLabelArea(QWidget *parent) : QWidget(parent) {
        setFixedHeight(24);
    }

    TierLabelArea::~dsfw::TierLabelArea() = default;

    void TierLabelArea::setBoundaryModel(IBoundaryModel *model) {
        m_boundaryModel = model;
        update();
    }

    IBoundaryModel *TierLabelArea::boundaryModel() const {
        return m_boundaryModel;
    }

    void TierLabelArea::setViewportController(dsfw::ViewportController *viewport) {
        if (m_viewport) {
            disconnect(m_viewport, nullptr, this, nullptr);
        }
        m_viewport = viewport;
        if (m_viewport) {
            m_viewStart = m_viewport->state().startSec;
            m_viewEnd = m_viewport->state().endSec;
            connect(m_viewport, &ViewportController::viewportChanged, this, [this](const ViewportState &state) {
                m_viewStart = state.startSec;
                m_viewEnd = state.endSec;
                update();
            });
        }
        update();
    }

    dsfw::ViewportController *TierLabelArea::viewportController() const {
        return m_viewport;
    }

    int TierLabelArea::activeTierIndex() const {
        return m_activeTierIndex;
    }

    void TierLabelArea::setActiveTierIndex(int index) {
        if (m_activeTierIndex != index) {
            m_activeTierIndex = index;
            emit activeTierChanged(index);
            update();
        }
    }

    QList<double> TierLabelArea::activeBoundaries() const {
        QList<double> boundaries;
        if (!m_boundaryModel)
            return boundaries;

        int tier = activeTierIndex();
        if (tier < 0 || tier >= m_boundaryModel->tierCount())
            return boundaries;

        int count = m_boundaryModel->boundaryCount(tier);
        boundaries.reserve(count);
        for (int i = 0; i < count; ++i)
            boundaries.append(usToSec(m_boundaryModel->boundaryTime(tier, i)));

        return boundaries;
    }

    double TierLabelArea::timeToX(double time) const {
        return buildCoordinate().timeToX(time);
    }

    dstools::ChartCoordinate TierLabelArea::buildCoordinate() const {
        dstools::ChartCoordinate coord;
        coord.viewStart = m_viewStart;
        coord.viewEnd = m_viewEnd;
        coord.pixelWidth = width();
        return coord;
    }

    void TierLabelArea::onModelDataChanged() {
        update();
    }

} // namespace dstools
