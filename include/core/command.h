/**
 * @file command.h
 * @brief Undoable command interface.
 * @author Laurent Jiang
 * @date 2025-12-08
 */

#pragma once

namespace gimp {

/*!
 * @class Command
 * @brief Abstract interface for undoable operations.
 */
class Command {
  public:
    virtual ~Command() = default;

    /*! @brief Executes the command. */
    virtual void apply() = 0;

    /*! @brief Reverts the command. */
    virtual void undo() = 0;
};
}  // namespace gimp
