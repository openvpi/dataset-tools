#include <dsfw/UndoStack.h>

namespace dstools {

void UndoStack::push(std::unique_ptr<ICommand> cmd) {
    // Remove any commands after current index (discard redo history)
    if (m_index < static_cast<int>(m_commands.size()))
        m_commands.erase(m_commands.begin() + m_index, m_commands.end());

    // Try to merge with previous command
    if (!m_commands.empty() && m_commands.back()->mergeWith(cmd.get())) {
        return; // Merged, discard new command
    }

    cmd->execute();
    m_commands.push_back(std::move(cmd));
    m_index = static_cast<int>(m_commands.size());
}

void UndoStack::undo() {
    if (!canUndo())
        return;
    --m_index;
    m_commands[m_index]->undo();
}

void UndoStack::redo() {
    if (!canRedo())
        return;
    m_commands[m_index]->execute();
    ++m_index;
}

void UndoStack::clear() {
    m_commands.clear();
    m_index = 0;
    m_cleanIndex = 0;
}

bool UndoStack::canUndo() const { return m_index > 0; }
bool UndoStack::canRedo() const { return m_index < static_cast<int>(m_commands.size()); }
QString UndoStack::undoText() const { return canUndo() ? m_commands[m_index - 1]->text() : QString(); }
QString UndoStack::redoText() const { return canRedo() ? m_commands[m_index]->text() : QString(); }
int UndoStack::count() const { return static_cast<int>(m_commands.size()); }
bool UndoStack::isClean() const { return m_index == m_cleanIndex; }
void UndoStack::setClean() { m_cleanIndex = m_index; }

} // namespace dstools
