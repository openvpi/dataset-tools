#include "MoveBoundaryCommand.h"
#include "../IBoundaryModel.h"

#include <QString>

namespace dstools {
namespace phonemelabeler {

MoveBoundaryCommand::MoveBoundaryCommand(IBoundaryModel *model, int tierIndex,
                                       int boundaryIndex, TimePos oldTime, TimePos newTime,
                                       QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_tierIndex(tierIndex)
    , m_boundaryIndex(boundaryIndex)
    , m_oldTime(oldTime)
    , m_newTime(newTime)
{
    setText(QString("Move boundary T%1:B%2").arg(tierIndex).arg(boundaryIndex));
}

void MoveBoundaryCommand::redo() {
    if (m_model) {
        m_model->moveBoundary(m_tierIndex, m_boundaryIndex, m_newTime);
    }
}

void MoveBoundaryCommand::undo() {
    if (m_model) {
        m_model->moveBoundary(m_tierIndex, m_boundaryIndex, m_oldTime);
    }
}

bool MoveBoundaryCommand::mergeWith(const QUndoCommand *other) {
    auto *o = dynamic_cast<const MoveBoundaryCommand *>(other);
    if (!o) return false;
    if (o->m_tierIndex != m_tierIndex || o->m_boundaryIndex != m_boundaryIndex)
        return false;
    m_newTime = o->m_newTime;
    return true;
}

} // namespace phonemelabeler
} // namespace dstools
