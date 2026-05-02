#pragma once

#include <QUndoCommand>
#include <vector>

#include <dstools/TimePos.h>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;
struct AlignedBoundary;

class LinkedMoveBoundaryCommand : public QUndoCommand {
public:
    LinkedMoveBoundaryCommand(TextGridDocument *doc,
                              int sourceTierIndex, int sourceBoundaryIndex,
                              TimePos oldTime, TimePos newTime,
                              const std::vector<AlignedBoundary> &aligned,
                              QUndoCommand *parent = nullptr);
};

} // namespace phonemelabeler
} // namespace dstools
