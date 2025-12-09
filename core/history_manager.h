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

class HistoryManager {
  public:
    virtual ~HistoryManager() = default;
    virtual void push(std::shared_ptr<Command> command) = 0;
    virtual bool undo() = 0;
    virtual bool redo() = 0;
};
}  // namespace gimp
