/// @file LinkedMoveBoundaryCommand.h
/// @brief Undo command for moving linked boundaries across multiple tiers.

#pragma once

#include <QUndoCommand>
#include <vector>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;
struct AlignedBoundary;

/// @brief Composite QUndoCommand that moves a source boundary and all aligned
///        boundaries together.
class LinkedMoveBoundaryCommand : public QUndoCommand {
public:
    /// @brief Constructs a linked move command.
    /// @param doc TextGrid document.
    /// @param sourceTierIndex Tier index of the source boundary.
    /// @param sourceBoundaryIndex Boundary index of the source boundary.
    /// @param oldTime Original time in seconds.
    /// @param newTime New time in seconds.
    /// @param aligned Vector of aligned boundaries to move together.
    /// @param parent Optional parent command.
    LinkedMoveBoundaryCommand(TextGridDocument *doc,
                              int sourceTierIndex, int sourceBoundaryIndex,
                              double oldTime, double newTime,
                              const std::vector<AlignedBoundary> &aligned,
                              QUndoCommand *parent = nullptr);
};

} // namespace phonemelabeler
} // namespace dstools
