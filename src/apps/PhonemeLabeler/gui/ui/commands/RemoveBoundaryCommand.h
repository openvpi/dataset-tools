/// @file RemoveBoundaryCommand.h
/// @brief Undo command for removing a boundary from a TextGrid tier.

#pragma once

#include <QUndoCommand>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;

/// @brief QUndoCommand that removes a boundary, saving its time for undo.
class RemoveBoundaryCommand : public QUndoCommand {
public:
    /// @brief Constructs a remove boundary command.
    /// @param doc TextGrid document.
    /// @param tierIndex Tier containing the boundary.
    /// @param boundaryIndex Index of the boundary to remove.
    /// @param parent Optional parent command.
    RemoveBoundaryCommand(TextGridDocument *doc, int tierIndex,
                          int boundaryIndex, QUndoCommand *parent = nullptr);

    void redo() override;
    void undo() override;

    /// @brief Returns the command ID for merge support.
    int id() const override { return 4; }

private:
    TextGridDocument *m_doc;    ///< Associated document.
    int m_tierIndex;            ///< Tier index.
    int m_boundaryIndex;        ///< Boundary index.
    double m_savedTime;         ///< Saved boundary time for undo.
};

} // namespace phonemelabeler
} // namespace dstools
