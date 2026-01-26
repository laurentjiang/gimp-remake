/**
 * @file history_manager.h
 * @brief Tracks undo/redo history.
 * @author Laurent Jiang
 * @date 2025-12-08
 */

#pragma once

#include <memory>

namespace gimp {
class Command;

/*!
 * @class HistoryManager
 * @brief Abstract interface for undo/redo history tracking.
 */
class HistoryManager {
  public:
    virtual ~HistoryManager() = default;

    /*!
     * @brief Pushes a command onto the history stack.
     * @param command The command to record.
     */
    virtual void push(std::shared_ptr<Command> command) = 0;

    /*!
     * @brief Undoes the last command.
     * @return True if an undo was performed.
     */
    virtual bool undo() = 0;

    /*!
     * @brief Redoes the last undone command.
     * @return True if a redo was performed.
     */
    virtual bool redo() = 0;
};
}  // namespace gimp
