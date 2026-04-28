#pragma once

#include <QUndoCommand>
#include <vector>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;
struct AlignedBoundary;

class LinkedMoveBoundaryCommand : public QUndoCommand {
public:
    LinkedMoveBoundaryCommand(TextGridDocument *doc,
                              int sourceTierIndex, int sourceBoundaryIndex,
                              double oldTime, double newTime,
                              const std::vector<AlignedBoundary> &aligned,
                              QUndoCommand *parent = nullptr);
};

} // namespace phonemelabeler
} // namespace dstools
