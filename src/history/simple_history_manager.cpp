/**
 * @file simple_history_manager.cpp
 * @brief Simple implementation of HistoryManager.
 * @author Aless Tosi
 * @date 2026-01-20
 */

#include "history/simple_history_manager.h"

#include "history/history_stack.h"

namespace gimp {
SimpleHistoryManager::SimpleHistoryManager() : stack_{std::make_shared<HistoryStack>()} {}

SimpleHistoryManager::SimpleHistoryManager(std::shared_ptr<HistoryStack> stack)
    : stack_{std::move(stack)}
{
}

void SimpleHistoryManager::push(std::shared_ptr<Command> command)
{
    if (stack_) {
        stack_->push(std::move(command));
    }
}

bool SimpleHistoryManager::undo()
{
    return stack_ && stack_->undo();
}

bool SimpleHistoryManager::redo()
{
    return stack_ && stack_->redo();
}

void SimpleHistoryManager::clear()
{
    if (stack_) {
        stack_->clear();
    }
}

bool SimpleHistoryManager::can_undo() const
{
    return stack_ && stack_->can_undo();
}

bool SimpleHistoryManager::can_redo() const
{
    return stack_ && stack_->can_redo();
}

size_t SimpleHistoryManager::undo_size() const
{
    return stack_ ? stack_->undo_size() : 0;
}

size_t SimpleHistoryManager::redo_size() const
{
    return stack_ ? stack_->redo_size() : 0;
}
}  // namespace gimp
