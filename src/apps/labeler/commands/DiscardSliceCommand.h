#pragma once

#include <QUndoCommand>
#include <QListWidget>

namespace dstools::labeler {

class DiscardSliceCommand : public QUndoCommand {
public:
    enum class State { Discard, Restore };

    DiscardSliceCommand(QListWidget *listWidget, int row, State state,
                        QUndoCommand *parent = nullptr);

    void undo() override;
    void redo() override;

private:
    void applyState(State state);
    static void setItemDiscarded(QListWidgetItem *item, bool discarded);

    QListWidget *m_listWidget;
    int m_row;
    State m_state;
};

} // namespace dstools::labeler
