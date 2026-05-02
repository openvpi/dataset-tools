#include "InsertBoundaryCommand.h"
#include "../TextGridDocument.h"

#include <cmath>

namespace dstools {
namespace phonemelabeler {

InsertBoundaryCommand::InsertBoundaryCommand(TextGridDocument *doc, int tierIndex,
                                            TimePos time, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_doc(doc)
    , m_tierIndex(tierIndex)
    , m_time(time)
    , m_insertedBoundaryIndex(-1)
{
    setText(QString("Insert boundary T%1 at %2ms").arg(tierIndex).arg(usToMs(time), 0, 'f', 1));
}

void InsertBoundaryCommand::redo() {
    if (m_doc) {
        m_doc->insertBoundary(m_tierIndex, m_time);
    }
}

void InsertBoundaryCommand::undo() {
    if (m_doc) {
        int count = m_doc->boundaryCount(m_tierIndex);
        constexpr TimePos kTolerance = 1000; // 1ms
        for (int i = 0; i < count; ++i) {
            if (std::abs(m_doc->boundaryTime(m_tierIndex, i) - m_time) < kTolerance) {
                m_doc->removeBoundary(m_tierIndex, i);
                break;
            }
        }
    }
}

} // namespace phonemelabeler
} // namespace dstools
