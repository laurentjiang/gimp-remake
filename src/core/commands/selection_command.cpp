/**
 * @file selection_command.cpp
 * @brief Implementation of SelectionCommand.
 * @author Laurent Jiang
 * @date 2026-02-09
 */

#include "core/commands/selection_command.h"

#include "core/event_bus.h"
#include "core/events.h"
#include "core/selection_manager.h"

#include <utility>

namespace gimp {

SelectionCommand::SelectionCommand(std::string description) : description_(std::move(description))
{
}

void SelectionCommand::captureBeforeState()
{
    beforePath_ = SelectionManager::instance().selectionPath();
    beforeType_ = SelectionManager::instance().selectionType();
}

void SelectionCommand::captureAfterState()
{
    afterPath_ = SelectionManager::instance().selectionPath();
    afterType_ = SelectionManager::instance().selectionType();
}

void SelectionCommand::apply()
{
    // Restore the after state
    SelectionManager::instance().restoreSelection(afterPath_, afterType_);

    // Publish selection changed event
    // NOLINTNEXTLINE(modernize-use-designated-initializers)
    EventBus::instance().publish(SelectionChangedEvent{!afterPath_.isEmpty(), "redo"});
}

void SelectionCommand::undo()
{
    // Restore the before state
    SelectionManager::instance().restoreSelection(beforePath_, beforeType_);

    // Publish selection changed event
    // NOLINTNEXTLINE(modernize-use-designated-initializers)
    EventBus::instance().publish(SelectionChangedEvent{!beforePath_.isEmpty(), "undo"});
}

}  // namespace gimp
