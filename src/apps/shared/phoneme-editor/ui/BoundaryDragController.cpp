#include "BoundaryDragController.h"
#include "IBoundaryModel.h"
#include "commands/BoundaryCommands.h"

#include <QUndoCommand>
#include <QUndoStack>

#include <cmath>

namespace dstools {
namespace phonemelabeler {

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

// ── Drag lifecycle ──────────────────────────────────────────────────────

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

    // Discover binding partners
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

    // Clamp source boundary
    TimePos clampedTime = m_model->clampBoundaryTime(
        m_draggedTier, m_draggedBoundary, proposedTime);

    // Preview-move source
    m_model->moveBoundary(m_draggedTier, m_draggedBoundary, clampedTime);

    // Move partners by the same delta
    TimePos delta = clampedTime - m_dragStartTime;
    for (size_t i = 0; i < m_partners.size(); ++i) {
        TimePos partnerProposed = m_partnerStartTimes[i] + delta;
        TimePos partnerClamped = m_model->clampBoundaryTime(
            m_partners[i].tierIndex, m_partners[i].boundaryIndex, partnerProposed);
        m_model->moveBoundary(m_partners[i].tierIndex,
                              m_partners[i].boundaryIndex, partnerClamped);
    }

    emit dragging(m_draggedTier, m_draggedBoundary, clampedTime);
}

void BoundaryDragController::endDrag(TimePos finalTime, QUndoStack *undoStack) {
    if (!m_dragging || !m_model)
        return;

    // Clamp final position
    TimePos clampedFinal = m_model->clampBoundaryTime(
        m_draggedTier, m_draggedBoundary, finalTime);

    // Snap if enabled
    if (m_snapEnabled) {
        clampedFinal = m_model->snapToNearestBoundary(
            m_draggedTier, clampedFinal, secToUs(0.01));
    }

    // Restore all to original positions before pushing undo command
    restoreOriginals();

    // Push undo command
    int tier = m_draggedTier;
    int boundary = m_draggedBoundary;

    if (undoStack) {
        if (m_partners.empty()) {
            undoStack->push(new MoveBoundaryCommand(
                m_model, tier, boundary, m_dragStartTime, clampedFinal));
        } else {
            // Composite command: source + all partners
            auto *parent = new QUndoCommand(
                QStringLiteral("Linked boundary move"));
            new MoveBoundaryCommand(m_model, tier, boundary,
                                    m_dragStartTime, clampedFinal, parent);
            TimePos delta = clampedFinal - m_dragStartTime;
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
        // No undo stack — apply directly
        m_model->moveBoundary(tier, boundary, clampedFinal);
        TimePos delta = clampedFinal - m_dragStartTime;
        for (size_t i = 0; i < m_partners.size(); ++i) {
            m_model->moveBoundary(m_partners[i].tierIndex,
                                  m_partners[i].boundaryIndex,
                                  m_partnerStartTimes[i] + delta);
        }
    }

    // Reset state
    m_dragging = false;
    m_draggedTier = -1;
    m_draggedBoundary = -1;
    m_partners.clear();
    m_partnerStartTimes.clear();
    m_model = nullptr;

    emit dragFinished(tier, boundary, clampedFinal);
}

void BoundaryDragController::cancelDrag() {
    if (!m_dragging || !m_model)
        return;

    restoreOriginals();

    m_dragging = false;
    m_draggedTier = -1;
    m_draggedBoundary = -1;
    m_partners.clear();
    m_partnerStartTimes.clear();
    m_model = nullptr;
}

// ── Private ─────────────────────────────────────────────────────────────

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

} // namespace phonemelabeler
} // namespace dstools
