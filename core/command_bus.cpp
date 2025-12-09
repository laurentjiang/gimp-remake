/**
 * @file command_bus.cpp
 * @brief Basic command bus implementation.
 * @author Laurent Jiang
 * @date 2025-12-09
 */

#include "command_bus.h"

#include "command.h"
#include "history_manager.h"

namespace gimp
{
BasicCommandBus::BasicCommandBus(HistoryManager& history) : history_{&history} {}

void BasicCommandBus::dispatch(std::shared_ptr<Command> command)
{
    if (!command || !history_)
    {
        return;
    }

    command->apply();
    history_->push(std::move(command));
}

HistoryManager& BasicCommandBus::history()
{
    return *history_;
}
} // namespace gimp
