#pragma once

/// @file UndoStack.h
/// @brief Lightweight command pattern with undo/redo support.

#include <QString>

#include <memory>
#include <vector>

namespace dstools {

/// @brief Abstract command interface.
class ICommand {
public:
    virtual ~ICommand() = default;
    /// @brief Execute (or re-execute) the command.
    virtual void execute() = 0;
    /// @brief Undo the command.
    virtual void undo() = 0;
    /// @brief Human-readable description.
    virtual QString text() const = 0;
    /// @brief Whether this command can merge with another.
    virtual bool mergeWith(const ICommand *) { return false; }
};

/// @brief Manages a stack of commands with undo/redo capability.
class UndoStack {
public:
    UndoStack() = default;

    /// @brief Push and execute a command.
    void push(std::unique_ptr<ICommand> cmd);
    /// @brief Undo the last command.
    void undo();
    /// @brief Redo the last undone command.
    void redo();
    /// @brief Clear all commands.
    void clear();

    bool canUndo() const;
    bool canRedo() const;
    QString undoText() const;
    QString redoText() const;
    int count() const;
    bool isClean() const;
    void setClean();

private:
    std::vector<std::unique_ptr<ICommand>> m_commands;
    int m_index = 0;
    int m_cleanIndex = 0;
};

} // namespace dstools
