/**
 * @file history_stack.h
 * @brief Undo/redo stack implementation for command history.
 * @author Aless Tosi
 * @date 2026-01-20
 */

#pragma once

#include <deque>
#include <memory>

namespace gimp {
class Command;

/**
 * @class HistoryStack
 * @brief Manages a stack of commands with undo/redo functionality.
 *
 * This class maintains two stacks: one for undo history and one for redo history.
 * Commands are pushed onto the undo stack, and undo/redo operations manage
 * movement between the two stacks.
 */
class HistoryStack {
  public:
    HistoryStack() = default;
    ~HistoryStack() = default;

    /**
     * @brief Push a command onto the undo stack.
     * Clears the redo stack when a new command is executed.
     *
     * @param command The command to push.
     */
    void push(std::shared_ptr<Command> command);

    /**
     * @brief Undo the last command.
     *
     * @return true if undo succeeded, false if undo stack is empty.
     */
    bool undo();

    /**
     * @brief Redo the last undone command.
     *
     * @return true if redo succeeded, false if redo stack is empty.
     */
    bool redo();

    /**
     * @brief Clear all history.
     */
    void clear();

    /**
     * @brief Check if undo is available.
     *
     * @return true if there are commands to undo.
     */
    [[nodiscard]] bool can_undo() const;

    /**
     * @brief Check if redo is available.
     *
     * @return true if there are commands to redo.
     */
    [[nodiscard]] bool can_redo() const;

    /**
     * @brief Get the size of the undo stack.
     *
     * @return The number of commands that can be undone.
     */
    [[nodiscard]] size_t undo_size() const;

    /**
     * @brief Get the size of the redo stack.
     *
     * @return The number of commands that can be redone.
     */
    [[nodiscard]] size_t redo_size() const;

  private:
    std::deque<std::shared_ptr<Command>> undo_stack_;
    std::deque<std::shared_ptr<Command>> redo_stack_;
};
}  // namespace gimp
