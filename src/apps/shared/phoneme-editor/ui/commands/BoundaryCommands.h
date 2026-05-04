/// @file BoundaryCommands.h
/// @brief Undo commands for boundary and interval operations in TextGrid tiers.

#pragma once

#include <QUndoCommand>
#include <QString>
#include <vector>

#include <dstools/TimePos.h>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;
class IBoundaryModel;
struct AlignedBoundary;

// ─── MoveBoundaryCommand ──────────────────────────────────────────────────────

class MoveBoundaryCommand : public QUndoCommand {
public:
    MoveBoundaryCommand(IBoundaryModel *model, int tierIndex,
                        int boundaryIndex, TimePos oldTime, TimePos newTime,
                        QUndoCommand *parent = nullptr);

    void redo() override;
    void undo() override;

    int id() const override { return 1; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    IBoundaryModel *m_model;
    int m_tierIndex;
    int m_boundaryIndex;
    TimePos m_oldTime;
    TimePos m_newTime;
};

// ─── SetIntervalTextCommand ───────────────────────────────────────────────────

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

// ─── InsertBoundaryCommand ────────────────────────────────────────────────────

class InsertBoundaryCommand : public QUndoCommand {
public:
    InsertBoundaryCommand(TextGridDocument *doc, int tierIndex,
                          TimePos time, QUndoCommand *parent = nullptr);

    void redo() override;
    void undo() override;

    int id() const override { return 3; }

private:
    TextGridDocument *m_doc;
    int m_tierIndex;
    TimePos m_time;
    int m_insertedBoundaryIndex;
};

// ─── RemoveBoundaryCommand ────────────────────────────────────────────────────

class RemoveBoundaryCommand : public QUndoCommand {
public:
    RemoveBoundaryCommand(TextGridDocument *doc, int tierIndex,
                          int boundaryIndex, QUndoCommand *parent = nullptr);

    void redo() override;
    void undo() override;

    int id() const override { return 4; }

private:
    TextGridDocument *m_doc;
    int m_tierIndex;
    int m_boundaryIndex;
    TimePos m_savedTime;
};

// ─── LinkedMoveBoundaryCommand ────────────────────────────────────────────────

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
