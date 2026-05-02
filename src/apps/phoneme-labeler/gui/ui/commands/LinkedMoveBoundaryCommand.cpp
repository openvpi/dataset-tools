#include "LinkedMoveBoundaryCommand.h"
#include "MoveBoundaryCommand.h"
#include "../TextGridDocument.h"
#include "../BoundaryBindingManager.h"

#include <QString>

namespace dstools {
namespace phonemelabeler {

LinkedMoveBoundaryCommand::LinkedMoveBoundaryCommand(
    TextGridDocument *doc,
    int sourceTierIndex, int sourceBoundaryIndex,
    TimePos oldTime, TimePos newTime,
    const std::vector<AlignedBoundary> &aligned,
    QUndoCommand *parent)
    : QUndoCommand("Linked boundary move", parent)
{
    TimePos delta = newTime - oldTime;

    new MoveBoundaryCommand(doc, sourceTierIndex, sourceBoundaryIndex,
                            oldTime, newTime, this);

    for (const auto &ab : aligned) {
        TimePos newAlignedTime = ab.time + delta;
        new MoveBoundaryCommand(doc, ab.tierIndex, ab.boundaryIndex,
                                ab.time, newAlignedTime, this);
    }
}

} // namespace phonemelabeler
} // namespace dstools
