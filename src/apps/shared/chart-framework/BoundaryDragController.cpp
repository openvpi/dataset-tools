#include "BoundaryDragController.h"
#include "IBoundaryModel.h"
#include "MoveBoundaryCommand.h"

#include <QUndoCommand>
#include <QUndoStack>

#include <algorithm>
#include <cmath>

namespace dstools {

using namespace dsfw;

using namespace dsfw;

BoundaryDragController::BoundaryDragController(QObject *parent)
    : QObject(parent) {
}

void BoundaryDragController::setBindingEnabled(bool enabled) {
    if (m_bindingEnabled != enabled) {
        m_bindingEnabled = enabled;
        emit bindingEnabledChanged(enabled);
    }
}

void BoundaryDragController::setSnapEnabled(bool enabled) {
    if (m_snapEnabled != enabled) {
        m_snapEnabled = enabled;
        emit snapEnabledChanged(enabled);
    }
}

void BoundaryDragController::setToleranceUs(TimePos toleranceUs) {
    m_toleranceUs = toleranceUs;
}

void BoundaryDragController::setSnapThresholdUs(TimePos thresholdUs) {
    m_snapThresholdUs = thresholdUs;
}

void BoundaryDragController::setCoordConverter(const ChartCoordinate *conv, int pixelWidth) {
    m_converter = conv;
    m_pixelWidth = pixelWidth;
}

void BoundaryDragController::setPixelSnapThreshold(double pixelThreshold) {
    m_pixelSnapThreshold = pixelThreshold;
}

void BoundaryDragController::startDrag(int tierIndex, int boundaryIndex,
                                        IBoundaryModel *model) {
    if (!model)
        return;

    if (m_dragging)
        cancelDrag();

    m_model = model;
    m_draggedTier = tierIndex;
    m_draggedBoundary = boundaryIndex;
    m_dragStartTime = model->boundaryTime(tierIndex, boundaryIndex);
    m_dragging = true;

    if (m_bindingEnabled) {
        m_partners = findPartners(tierIndex, boundaryIndex);
        m_partnerStartTimes.clear();
        m_partnerStartTimes.reserve(m_partners.size());
        for (const auto &p : m_partners)
            m_partnerStartTimes.push_back(p.time);
    } else {
        m_partners.clear();
        m_partnerStartTimes.clear();
    }

    emit dragStarted(tierIndex, boundaryIndex);
}

void BoundaryDragController::updateDrag(TimePos proposedTime) {
    if (!m_dragging || !m_model)
        return;

    TimePos clampedTime = m_model->clampBoundaryTime(
        m_draggedTier, m_draggedBoundary, proposedTime);

    TimePos tentativeDelta = clampedTime - m_dragStartTime;
    for (size_t i = 0; i < m_partners.size(); ++i) {
        TimePos partnerTarget = m_partnerStartTimes[i] + tentativeDelta;
        m_model->moveBoundary(m_partners[i].tierIndex,
                              m_partners[i].boundaryIndex, partnerTarget);
    }

    m_model->moveBoundary(m_draggedTier, m_draggedBoundary, clampedTime);

    TimePos snappedTime = clampedTime;
    if (m_snapEnabled && clampedTime == proposedTime) {
        snappedTime = m_model->snapToNearestBoundaryPixels(
            m_draggedTier, clampedTime, PPS(), m_pixelSnapThreshold);
    }

    if (snappedTime != clampedTime) {
        m_model->moveBoundary(m_draggedTier, m_draggedBoundary, snappedTime);
        TimePos snapDelta = snappedTime - clampedTime;
        for (size_t i = 0; i < m_partners.size(); ++i) {
            TimePos partnerTarget = m_partnerStartTimes[i] + tentativeDelta + snapDelta;
            m_model->moveBoundary(m_partners[i].tierIndex,
                                  m_partners[i].boundaryIndex, partnerTarget);
        }
    }

    emit dragging(m_draggedTier, m_draggedBoundary, snappedTime);
}

void BoundaryDragController::endDrag(TimePos finalTime, QUndoStack *undoStack) {
    if (!m_dragging || !m_model)
        return;

    TimePos clampedFinal = m_model->clampBoundaryTime(
        m_draggedTier, m_draggedBoundary, finalTime);

    TimePos snappedFinal = clampedFinal;
    if (m_snapEnabled && clampedFinal == finalTime) {
        snappedFinal = m_model->snapToNearestBoundaryPixels(
            m_draggedTier, clampedFinal, PPS(), m_pixelSnapThreshold);
    }

    snappedFinal = std::clamp(snappedFinal, TimePos(0), m_model->totalDuration());

    restoreOriginals();

    int tier = m_draggedTier;
    int boundary = m_draggedBoundary;

    if (undoStack) {
        if (m_partners.empty()) {
            undoStack->push(new MoveBoundaryCommand(
                m_model, tier, boundary, m_dragStartTime, snappedFinal));
        } else {
            auto *parent = new QUndoCommand(
                QStringLiteral("Linked boundary move"));
            new MoveBoundaryCommand(m_model, tier, boundary,
                                    m_dragStartTime, snappedFinal, parent);
            TimePos delta = snappedFinal - m_dragStartTime;
            for (size_t i = 0; i < m_partners.size(); ++i) {
                TimePos newPartnerTime = m_partnerStartTimes[i] + delta;
                new MoveBoundaryCommand(m_model,
                                        m_partners[i].tierIndex,
                                        m_partners[i].boundaryIndex,
                                        m_partnerStartTimes[i],
                                        newPartnerTime, parent);
            }
            undoStack->push(parent);
        }
    } else {
        m_model->moveBoundary(tier, boundary, snappedFinal);
        TimePos delta = snappedFinal - m_dragStartTime;
        for (size_t i = 0; i < m_partners.size(); ++i) {
            m_model->moveBoundary(m_partners[i].tierIndex,
                                  m_partners[i].boundaryIndex,
                                  m_partnerStartTimes[i] + delta);
        }
    }

    m_dragging = false;
    m_draggedTier = -1;
    m_draggedBoundary = -1;
    m_partners.clear();
    m_partnerStartTimes.clear();
    m_model = nullptr;

    emit dragFinished(tier, boundary, snappedFinal);
}

void BoundaryDragController::cancelDrag() {
    if (!m_dragging || !m_model)
        return;

    restoreOriginals();

    int tier = m_draggedTier;
    int boundary = m_draggedBoundary;
    TimePos startTime = m_dragStartTime;

    m_dragging = false;
    m_draggedTier = -1;
    m_draggedBoundary = -1;
    m_partners.clear();
    m_partnerStartTimes.clear();
    m_model = nullptr;

    emit dragFinished(tier, boundary, startTime);
}

void BoundaryDragController::restoreOriginals() {
    if (!m_model)
        return;
    m_model->moveBoundary(m_draggedTier, m_draggedBoundary, m_dragStartTime);
    for (size_t i = 0; i < m_partners.size(); ++i) {
        m_model->moveBoundary(m_partners[i].tierIndex,
                              m_partners[i].boundaryIndex,
                              m_partnerStartTimes[i]);
    }
}

std::vector<AlignedBoundary> BoundaryDragController::findPartners(
    int sourceTier, int sourceBoundaryIndex) const {
    std::vector<AlignedBoundary> result;
    if (!m_model)
        return result;

    TimePos sourceTime = m_model->boundaryTime(sourceTier, sourceBoundaryIndex);

    int tierCount = m_model->tierCount();
    for (int t = 0; t < tierCount; ++t) {
        if (t == sourceTier)
            continue;

        int bCount = m_model->boundaryCount(t);
        for (int b = 0; b < bCount; ++b) {
            TimePos bTime = m_model->boundaryTime(t, b);
            if (std::abs(bTime - sourceTime) <= m_toleranceUs) {
                result.push_back({t, b, bTime});
            }
        }
    }

    return result;
}

} // namespace dstools