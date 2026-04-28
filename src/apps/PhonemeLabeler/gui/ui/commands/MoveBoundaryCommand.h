#pragma once

#include <QUndoCommand>

namespace dstools {
namespace phonemelabeler {

class TextGridDocument;

class MoveBoundaryCommand : public QUndoCommand {
public:
    MoveBoundaryCommand(TextGridDocument *doc, int tierIndex,
                        int boundaryIndex, double oldTime, double newTime,
                        QUndoCommand *parent = nullptr);

    void redo() override;
    void undo() override;

    int id() const override { return 1; }

    // Merge consecutive moves of the same boundary
    bool mergeWith(const QUndoCommand *other) override;

private:
    TextGridDocument *m_doc;
    int m_tierIndex;
    int m_boundaryIndex;
    double m_oldTime;
    double m_newTime;
};

} // namespace phonemelabeler
} // namespace dstools
