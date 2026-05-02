#pragma once

#include <QUndoCommand>

#include <dstools/TimePos.h>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;

class MoveBoundaryCommand : public QUndoCommand {
public:
    MoveBoundaryCommand(TextGridDocument *doc, int tierIndex,
                        int boundaryIndex, TimePos oldTime, TimePos newTime,
                        QUndoCommand *parent = nullptr);

    void redo() override;
    void undo() override;

    int id() const override { return 1; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    TextGridDocument *m_doc;
    int m_tierIndex;
    int m_boundaryIndex;
    TimePos m_oldTime;
    TimePos m_newTime;
};

} // namespace phonemelabeler
} // namespace dstools
