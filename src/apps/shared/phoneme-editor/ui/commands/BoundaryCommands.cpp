#include "BoundaryCommands.h"
#include "../TextGridDocument.h"

#include <QString>
#include <cmath>

namespace dstools {
namespace phonemelabeler {

// ─── SetIntervalTextCommand ───────────────────────────────────────────────────

SetIntervalTextCommand::SetIntervalTextCommand(TextGridDocument *doc, int tierIndex,
                                               int intervalIndex, const QString &oldText,
                                               const QString &newText, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_doc(doc)
    , m_tierIndex(tierIndex)
    , m_intervalIndex(intervalIndex)
    , m_oldText(oldText)
    , m_newText(newText)
{
    setText(QString("Set interval text T%1:I%2").arg(tierIndex).arg(intervalIndex));
}

void SetIntervalTextCommand::redo() {
    if (m_doc) {
        m_doc->setIntervalText(m_tierIndex, m_intervalIndex, m_newText);
    }
}

void SetIntervalTextCommand::undo() {
    if (m_doc) {
        m_doc->setIntervalText(m_tierIndex, m_intervalIndex, m_oldText);
    }
}

bool SetIntervalTextCommand::mergeWith(const QUndoCommand *other) {
    if (other->id() != id())
        return false;
    auto *o = static_cast<const SetIntervalTextCommand *>(other);
    if (o->m_tierIndex != m_tierIndex || o->m_intervalIndex != m_intervalIndex)
        return false;
    m_newText = o->m_newText;
    return true;
}

// ─── InsertBoundaryCommand ────────────────────────────────────────────────────

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

// ─── RemoveBoundaryCommand ────────────────────────────────────────────────────

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
