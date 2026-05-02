#include "RemoveBoundaryCommand.h"
#include "../TextGridDocument.h"

namespace dstools {
namespace phonemelabeler {

RemoveBoundaryCommand::RemoveBoundaryCommand(TextGridDocument *doc, int tierIndex,
                                            int boundaryIndex, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_doc(doc)
    , m_tierIndex(tierIndex)
    , m_boundaryIndex(boundaryIndex)
    , m_savedTime(0)
{
    setText(QString("Remove boundary T%1:B%2").arg(tierIndex).arg(boundaryIndex));
}

void RemoveBoundaryCommand::redo() {
    if (m_doc) {
        m_savedTime = m_doc->boundaryTime(m_tierIndex, m_boundaryIndex);
        m_doc->removeBoundary(m_tierIndex, m_boundaryIndex);
    }
}

void RemoveBoundaryCommand::undo() {
    if (m_doc) {
        m_doc->insertBoundary(m_tierIndex, m_savedTime);
    }
}

} // namespace phonemelabeler
} // namespace dstools
