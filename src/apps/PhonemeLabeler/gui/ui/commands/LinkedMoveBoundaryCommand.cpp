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
    double oldTime, double newTime,
    const std::vector<AlignedBoundary> &aligned,
    QUndoCommand *parent)
    : QUndoCommand("Linked boundary move", parent)
{
    double delta = newTime - oldTime;

    // Source boundary as first child command
    new MoveBoundaryCommand(doc, sourceTierIndex, sourceBoundaryIndex,
                            oldTime, newTime, this);

    // Each aligned boundary as child command
    for (const auto &ab : aligned) {
        double newAlignedTime = ab.time + delta;
        new MoveBoundaryCommand(doc, ab.tierIndex, ab.boundaryIndex,
                                ab.time, newAlignedTime, this);
    }
}

} // namespace phonemelabeler
} // namespace dstools
