#include "DiscardSliceCommand.h"

#include <QBrush>

namespace dstools::labeler {

DiscardSliceCommand::DiscardSliceCommand(QListWidget *listWidget, int row, State state,
                                         QUndoCommand *parent)
    : QUndoCommand(parent), m_listWidget(listWidget), m_row(row), m_state(state) {
    if (state == State::Discard)
        setText(QObject::tr("Discard slice"));
    else
        setText(QObject::tr("Restore slice"));
}

void DiscardSliceCommand::undo() {
    applyState(m_state == State::Discard ? State::Restore : State::Discard);
}

void DiscardSliceCommand::redo() {
    applyState(m_state);
}

void DiscardSliceCommand::applyState(State state) {
    if (!m_listWidget || m_row < 0 || m_row >= m_listWidget->count())
        return;

    QListWidgetItem *item = m_listWidget->item(m_row);
    if (!item)
        return;

    setItemDiscarded(item, state == State::Discard);
}

void DiscardSliceCommand::setItemDiscarded(QListWidgetItem *item, bool discarded) {
    QFont font = item->font();
    font.setStrikeOut(discarded);
    item->setFont(font);

    QBrush brush = discarded ? QBrush(Qt::gray) : QBrush(Qt::black);
    item->setForeground(brush);

    item->setData(Qt::UserRole + 2, discarded);
}

} // namespace dstools::labeler
