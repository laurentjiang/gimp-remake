/**
 * @file selection_tool_base.cpp
 * @brief Implementation of SelectionToolBase.
 * @author Laurent Jiang
 * @date 2026-02-10
 */

#include "core/tools/selection_tool_base.h"

#include "core/command_bus.h"
#include "core/commands/selection_command.h"
#include "core/selection_manager.h"

namespace gimp {

SelectionMode SelectionToolBase::resolveSelectionMode(Qt::KeyboardModifiers modifiers)
{
    if ((modifiers & Qt::ControlModifier) != 0) {
        if ((modifiers & Qt::AltModifier) != 0) {
            return SelectionMode::Subtract;
        }
        return SelectionMode::Add;
    }
    return SelectionMode::Replace;
}

void SelectionToolBase::beginSelectionCommand(const std::string& description)
{
    pendingCommand_ = std::make_shared<SelectionCommand>(description);
    pendingCommand_->captureBeforeState();
}

void SelectionToolBase::commitSelectionCommand()
{
    if (pendingCommand_ && commandBus_) {
        pendingCommand_->captureAfterState();
        commandBus_->dispatch(pendingCommand_);
    }
    pendingCommand_.reset();
}

void SelectionToolBase::cancelSelectionOperation()
{
    SelectionManager::instance().clearPreview();
    pendingCommand_.reset();
}

}  // namespace gimp
