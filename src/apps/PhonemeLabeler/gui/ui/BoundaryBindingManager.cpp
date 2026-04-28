#include "BoundaryBindingManager.h"
#include "TextGridDocument.h"
#include "commands/MoveBoundaryCommand.h"

#include <algorithm>
#include <cmath>

namespace dstools {
namespace phonemelabeler {

BoundaryBindingManager::BoundaryBindingManager(TextGridDocument *doc, QObject *parent)
    : QObject(parent),
    m_document(doc)
{
}

void BoundaryBindingManager::setTolerance(double toleranceMs) {
    if (std::abs(m_tolerance - toleranceMs) > 0.001) {
        m_tolerance = toleranceMs;
        emit bindingChanged();
    }
}

void BoundaryBindingManager::setEnabled(bool enabled) {
    if (m_enabled != enabled) {
        m_enabled = enabled;
        emit bindingChanged();
    }
}

std::vector<AlignedBoundary> BoundaryBindingManager::findAlignedBoundaries(
    int sourceTierIndex, int sourceBoundaryIndex) const
{
    std::vector<AlignedBoundary> result;

    if (!m_enabled || !m_document || sourceTierIndex < 0)
        return result;

    // Get source boundary time
    double sourceTime = m_document->boundaryTime(sourceTierIndex, sourceBoundaryIndex);
    double tolSec = m_tolerance / 1000.0; // ms -> sec

    int tierCount = m_document->tierCount();
    for (int tier = 0; tier < tierCount; ++tier) {
        if (tier == sourceTierIndex)
            continue;
        if (!m_document->isIntervalTier(tier))
            continue;

        int count = m_document->boundaryCount(tier);
        for (int b = 0; b < count; ++b) {
            double t = m_document->boundaryTime(tier, b);
            if (t < sourceTime - tolSec)
                continue;
            if (t > sourceTime + tolSec)
                break; // boundaries are sorted, can exit early

            // Within tolerance - consider aligned
            if (std::abs(t - sourceTime) <= tolSec) {
                result.push_back({tier, b, t});
            }
        }
    }

    return result;
}

QUndoCommand *BoundaryBindingManager::createLinkedMoveCommand(
    int sourceTierIndex, int sourceBoundaryIndex,
    double newTime, TextGridDocument *doc)
{
    if (!doc) return nullptr;

    double oldTime = doc->boundaryTime(sourceTierIndex, sourceBoundaryIndex);
    auto aligned = findAlignedBoundaries(sourceTierIndex, sourceBoundaryIndex);

    if (aligned.empty()) {
        // No aligned boundaries, use simple move
        return new MoveBoundaryCommand(doc, sourceTierIndex, sourceBoundaryIndex,
                                       oldTime, newTime);
    }

    // Create linked move command with children
    auto *parentCmd = new QUndoCommand("Linked boundary move");

    // Source boundary as first child
    new MoveBoundaryCommand(doc, sourceTierIndex, sourceBoundaryIndex,
                           oldTime, newTime, parentCmd);

    // Each aligned boundary as child
    double delta = newTime - oldTime;
    for (const auto &ab : aligned) {
        double newAlignedTime = ab.time + delta;
        new MoveBoundaryCommand(doc, ab.tierIndex, ab.boundaryIndex,
                               ab.time, newAlignedTime, parentCmd);
    }

    return parentCmd;
}

} // namespace phonemelabeler
} // namespace dstools
