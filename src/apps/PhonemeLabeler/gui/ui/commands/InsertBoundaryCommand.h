/// @file InsertBoundaryCommand.h
/// @brief Undo command for inserting a boundary into a TextGrid tier.

#pragma once

#include <QUndoCommand>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;

/// @brief QUndoCommand that inserts a boundary at a given time position.
class InsertBoundaryCommand : public QUndoCommand {
public:
    /// @brief Constructs an insert boundary command.
    /// @param doc TextGrid document.
    /// @param tierIndex Tier to insert the boundary into.
    /// @param time Time position in seconds.
    /// @param parent Optional parent command.
    InsertBoundaryCommand(TextGridDocument *doc, int tierIndex,
                          double time, QUndoCommand *parent = nullptr);

    void redo() override;
    void undo() override;

    /// @brief Returns the command ID for merge support.
    int id() const override { return 3; }

private:
    TextGridDocument *m_doc;          ///< Associated document.
    int m_tierIndex;                  ///< Target tier index.
    double m_time;                    ///< Insertion time in seconds.
    int m_insertedBoundaryIndex;      ///< Index of the inserted boundary, set after redo.
};

} // namespace phonemelabeler
} // namespace dstools
