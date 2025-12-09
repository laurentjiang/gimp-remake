/**
 * @file command_bus.h
 * @brief Interface for dispatching undoable commands.
 * @author Laurent Jiang
 * @date 2025-12-09
 */

#pragma once

#include <memory>

namespace gimp
{
class Command;
class HistoryManager;

class CommandBus
{
public:
    virtual ~CommandBus() = default;

    virtual void dispatch(std::shared_ptr<Command> command) = 0;
    virtual HistoryManager& history() = 0;
};
} // namespace gimp
