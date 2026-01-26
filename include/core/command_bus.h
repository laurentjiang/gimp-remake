/**
 * @file command_bus.h
 * @brief Interface for dispatching undoable commands.
 * @author Laurent Jiang
 * @date 2025-12-09
 */

#pragma once

#include <memory>

namespace gimp {
class Command;
class HistoryManager;

/*!
 * @class CommandBus
 * @brief Abstract interface for dispatching commands to the system.
 */
class CommandBus {
  public:
    virtual ~CommandBus() = default;

    /*!
     * @brief Dispatches a command for execution.
     * @param command The command to execute.
     */
    virtual void dispatch(std::shared_ptr<Command> command) = 0;

    /*! @brief Returns the history manager for undo/redo.
     *  @return Reference to the history manager.
     */
    virtual HistoryManager& history() = 0;
};

/*!
 * @class BasicCommandBus
 * @brief Default implementation that executes commands and records history.
 */
class BasicCommandBus final : public CommandBus {
  public:
    /*!
     * @brief Constructs a command bus with a history manager.
     * @param history The history manager for undo/redo tracking.
     */
    explicit BasicCommandBus(HistoryManager& history);

    void dispatch(std::shared_ptr<Command> command) override;
    HistoryManager& history() override;

  private:
    HistoryManager* history_;
};
}  // namespace gimp
