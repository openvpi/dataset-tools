#include "BoundaryCommands.h"
#include "../TextGridDocument.h"
#include "../IBoundaryModel.h"
#include "../BoundaryBindingManager.h"

#include <QString>
#include <cmath>

namespace dstools {
namespace phonemelabeler {

// ─── MoveBoundaryCommand ──────────────────────────────────────────────────────

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
    auto *o = dynamic_cast<const SetIntervalTextCommand *>(other);
    if (!o) return false;
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

// ─── LinkedMoveBoundaryCommand ────────────────────────────────────────────────

LinkedMoveBoundaryCommand::LinkedMoveBoundaryCommand(
    TextGridDocument *doc,
    int sourceTierIndex, int sourceBoundaryIndex,
    TimePos oldTime, TimePos newTime,
    const std::vector<AlignedBoundary> &aligned,
    QUndoCommand *parent)
    : QUndoCommand("Linked boundary move", parent)
{
    TimePos delta = newTime - oldTime;

    new MoveBoundaryCommand(doc, sourceTierIndex, sourceBoundaryIndex,
                            oldTime, newTime, this);

    for (const auto &ab : aligned) {
        TimePos newAlignedTime = ab.time + delta;
        new MoveBoundaryCommand(doc, ab.tierIndex, ab.boundaryIndex,
                                ab.time, newAlignedTime, this);
    }
}

} // namespace phonemelabeler
} // namespace dstools
