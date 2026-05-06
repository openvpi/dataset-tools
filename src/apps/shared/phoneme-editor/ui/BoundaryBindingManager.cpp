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

    // Use BindingGroups from TextGridDocument (auto-detected or from FA).
    // Each group contains boundaries at the exact same TimePos.
    int sourceId = sourceTierIndex * 10000 + sourceBoundaryIndex + 1;
    int groupIdx = m_document->findGroupForBoundary(sourceId);
    if (groupIdx < 0)
        return result;

    const auto &groups = m_document->bindingGroups();
    if (groupIdx >= static_cast<int>(groups.size()))
        return result;

    for (int id : groups[groupIdx]) {
        if (id == sourceId)
            continue;

        // Decode synthetic ID back to (tier, index)
        int tier = (id - 1) / 10000;
        int index = (id - 1) % 10000;
        if (tier < 0 || tier >= m_document->tierCount())
            continue;
        if (index < 0 || index >= m_document->boundaryCount(tier))
            continue;

        TimePos t = m_document->boundaryTime(tier, index);
        result.push_back({tier, index, t});
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
