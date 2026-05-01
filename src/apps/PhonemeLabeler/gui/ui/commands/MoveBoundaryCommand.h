/// @file MoveBoundaryCommand.h
/// @brief Undo command for moving a single boundary in a TextGrid tier.

#pragma once

#include <QUndoCommand>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;

/// @brief QUndoCommand with merge support for consecutive moves of the same boundary.
class MoveBoundaryCommand : public QUndoCommand {
public:
    /// @brief Constructs a move boundary command.
    /// @param doc TextGrid document.
    /// @param tierIndex Tier containing the boundary.
    /// @param boundaryIndex Index of the boundary to move.
    /// @param oldTime Original time in seconds.
    /// @param newTime New time in seconds.
    /// @param parent Optional parent command.
    MoveBoundaryCommand(TextGridDocument *doc, int tierIndex,
                        int boundaryIndex, double oldTime, double newTime,
                        QUndoCommand *parent = nullptr);

    void redo() override;
    void undo() override;

    /// @brief Returns the command ID for merge support.
    int id() const override { return 1; }

    /// @brief Merges consecutive moves of the same boundary into one command.
    /// @param other Command to merge with.
    /// @return True if merged successfully.
    bool mergeWith(const QUndoCommand *other) override;

private:
    TextGridDocument *m_doc;    ///< Associated document.
    int m_tierIndex;            ///< Tier index.
    int m_boundaryIndex;        ///< Boundary index.
    double m_oldTime;           ///< Original time in seconds.
    double m_newTime;           ///< New time in seconds.
};

} // namespace phonemelabeler
} // namespace dstools
