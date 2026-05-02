#include "SetIntervalTextCommand.h"
#include "../TextGridDocument.h"

namespace dstools {
namespace phonemelabeler {

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

} // namespace phonemelabeler
} // namespace dstools
