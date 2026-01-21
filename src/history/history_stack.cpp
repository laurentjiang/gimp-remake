/**
 * @file history_stack.cpp
 * @brief Undo/redo stack implementation for command history.
 * @author Aless Tosi
 * @date 2026-01-20
 */

#include "history/history_stack.h"
#include "core/command.h"

namespace gimp {
void HistoryStack::push(std::shared_ptr<Command> command)
{
    if (!command) {
        return;
    }

    undo_stack_.push_back(std::move(command));
    redo_stack_.clear();
}

bool HistoryStack::undo()
{
    if (undo_stack_.empty()) {
        return false;
    }

    auto command = undo_stack_.back();
    undo_stack_.pop_back();

    command->undo();
    redo_stack_.push_back(std::move(command));

    return true;
}

bool HistoryStack::redo()
{
    if (redo_stack_.empty()) {
        return false;
    }

    auto command = redo_stack_.back();
    redo_stack_.pop_back();

    command->apply();
    undo_stack_.push_back(std::move(command));

    return true;
}

void HistoryStack::clear()
{
    undo_stack_.clear();
    redo_stack_.clear();
}

bool HistoryStack::can_undo() const
{
    return !undo_stack_.empty();
}

bool HistoryStack::can_redo() const
{
    return !redo_stack_.empty();
}

size_t HistoryStack::undo_size() const
{
    return undo_stack_.size();
}

size_t HistoryStack::redo_size() const
{
    return redo_stack_.size();
}
}  // namespace gimp
