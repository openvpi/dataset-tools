/// @file SetIntervalTextCommand.h
/// @brief Undo command for changing interval text in a TextGrid tier.

#pragma once

#include <QUndoCommand>
#include <QString>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;

/// @brief QUndoCommand with merge support for consecutive edits to the same interval's text.
class SetIntervalTextCommand : public QUndoCommand {
public:
    /// @brief Constructs a set interval text command.
    /// @param doc TextGrid document.
    /// @param tierIndex Tier containing the interval.
    /// @param intervalIndex Index of the interval to modify.
    /// @param oldText Previous text value.
    /// @param newText New text value.
    /// @param parent Optional parent command.
    SetIntervalTextCommand(TextGridDocument *doc, int tierIndex,
                           int intervalIndex, const QString &oldText,
                           const QString &newText, QUndoCommand *parent = nullptr);

    void redo() override;
    void undo() override;

    /// @brief Returns the command ID for merge support.
    int id() const override { return 2; }

    /// @brief Merges consecutive text edits to the same interval.
    /// @param other Command to merge with.
    /// @return True if merged successfully.
    bool mergeWith(const QUndoCommand *other) override;

private:
    TextGridDocument *m_doc;    ///< Associated document.
    int m_tierIndex;            ///< Tier index.
    int m_intervalIndex;        ///< Interval index.
    QString m_oldText;          ///< Previous text value.
    QString m_newText;          ///< New text value.
};

} // namespace phonemelabeler
} // namespace dstools
