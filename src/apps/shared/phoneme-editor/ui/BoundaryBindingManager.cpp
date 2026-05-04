#include "BoundaryBindingManager.h"
#include "TextGridDocument.h"
#include "IBoundaryModel.h"
#include "commands/BoundaryCommands.h"

#include <algorithm>
#include <cmath>

namespace dstools {
namespace phonemelabeler {

BoundaryBindingManager::BoundaryBindingManager(TextGridDocument *doc, QObject *parent)
    : QObject(parent),
    m_document(doc)
{
}

void BoundaryBindingManager::setToleranceMs(double toleranceMs) {
    TimePos newTol = msToUs(toleranceMs);
    if (m_toleranceUs != newTol) {
        m_toleranceUs = newTol;
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

    TimePos sourceTime = m_document->boundaryTime(sourceTierIndex, sourceBoundaryIndex);

    int tierCount = m_document->tierCount();
    for (int tier = 0; tier < tierCount; ++tier) {
        if (tier == sourceTierIndex)
            continue;
        if (!m_document->isIntervalTier(tier))
            continue;

        int count = m_document->boundaryCount(tier);
        for (int b = 0; b < count; ++b) {
            TimePos t = m_document->boundaryTime(tier, b);
            if (t < sourceTime - m_toleranceUs)
                continue;
            if (t > sourceTime + m_toleranceUs)
                break;

            if (std::abs(t - sourceTime) <= m_toleranceUs) {
                result.push_back({tier, b, t});
            }
        }
    }

    return result;
}

QUndoCommand *BoundaryBindingManager::createLinkedMoveCommand(
    int sourceTierIndex, int sourceBoundaryIndex,
    TimePos newTime, IBoundaryModel *model)
{
    if (!model) return nullptr;

    TimePos oldTime = model->boundaryTime(sourceTierIndex, sourceBoundaryIndex);
    auto aligned = findAlignedBoundaries(sourceTierIndex, sourceBoundaryIndex);

    if (aligned.empty()) {
        return new MoveBoundaryCommand(model, sourceTierIndex, sourceBoundaryIndex,
                                       oldTime, newTime);
    }

    auto *parentCmd = new QUndoCommand("Linked boundary move");

    new MoveBoundaryCommand(model, sourceTierIndex, sourceBoundaryIndex,
                           oldTime, newTime, parentCmd);

    TimePos delta = newTime - oldTime;
    for (const auto &ab : aligned) {
        TimePos newAlignedTime = ab.time + delta;
        new MoveBoundaryCommand(model, ab.tierIndex, ab.boundaryIndex,
                               ab.time, newAlignedTime, parentCmd);
    }

    return parentCmd;
}

} // namespace phonemelabeler
} // namespace dstools
