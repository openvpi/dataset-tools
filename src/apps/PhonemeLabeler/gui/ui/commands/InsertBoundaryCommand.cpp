#include "InsertBoundaryCommand.h"
#include "../TextGridDocument.h"

#include <cmath>

namespace dstools {
namespace phonemelabeler {

InsertBoundaryCommand::InsertBoundaryCommand(TextGridDocument *doc, int tierIndex,
                                            double time, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_doc(doc)
    , m_tierIndex(tierIndex)
    , m_time(time)
    , m_insertedBoundaryIndex(-1)
{
    setText(QString("Insert boundary T%1 at %2s").arg(tierIndex).arg(time, 0, 'f', 3));
}

void InsertBoundaryCommand::redo() {
    if (m_doc) {
        m_doc->insertBoundary(m_tierIndex, m_time);
    }
}

void InsertBoundaryCommand::undo() {
    if (m_doc) {
        // Remove the boundary we just inserted
        // The boundary index after insertion is the index for time
        int count = m_doc->boundaryCount(m_tierIndex);
        // Find the boundary closest to m_time
        for (int i = 0; i < count; ++i) {
            if (std::abs(m_doc->boundaryTime(m_tierIndex, i) - m_time) < 0.001) {
                m_doc->removeBoundary(m_tierIndex, i);
                break;
            }
        }
    }
}

} // namespace phonemelabeler
} // namespace dstools
