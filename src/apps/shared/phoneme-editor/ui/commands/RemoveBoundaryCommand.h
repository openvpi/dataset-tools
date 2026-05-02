#pragma once

#include <QUndoCommand>

#include <dstools/TimePos.h>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;

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

} // namespace phonemelabeler
} // namespace dstools
