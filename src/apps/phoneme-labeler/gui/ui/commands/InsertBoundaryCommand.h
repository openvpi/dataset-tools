#pragma once

#include <QUndoCommand>

#include <dstools/TimePos.h>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;

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

} // namespace phonemelabeler
} // namespace dstools
