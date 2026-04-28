#pragma once

#include <QUndoCommand>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;

class InsertBoundaryCommand : public QUndoCommand {
public:
    InsertBoundaryCommand(TextGridDocument *doc, int tierIndex,
                          double time, QUndoCommand *parent = nullptr);

    void redo() override;
    void undo() override;

    int id() const override { return 3; }

private:
    TextGridDocument *m_doc;
    int m_tierIndex;
    double m_time;
    int m_insertedBoundaryIndex; // set after redo
};

} // namespace phonemelabeler
} // namespace dstools
