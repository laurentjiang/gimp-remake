/**
 * @file simple_history_manager.h
 * @brief Simple implementation of HistoryManager.
 * @author Aless Tosi
 * @date 2026-01-20
 */

#pragma once

#include <memory>

#include "core/history_manager.h"

namespace gimp {
class HistoryStack;

/**
 * @class SimpleHistoryManager
 * @brief Concrete implementation of HistoryManager using HistoryStack.
 *
 * This manager wraps a HistoryStack to provide command history management
 * with undo/redo functionality.
 */
class SimpleHistoryManager final : public HistoryManager {
  public:
    /**
     * @brief Construct a SimpleHistoryManager with a new history stack.
     */
    SimpleHistoryManager();

    /**
     * @brief Construct a SimpleHistoryManager with an existing history stack.
     *
     * @param stack The history stack to use.
     */
    explicit SimpleHistoryManager(std::shared_ptr<HistoryStack> stack);

    ~SimpleHistoryManager() override = default;

    void push(std::shared_ptr<Command> command) override;
    bool undo() override;
    bool redo() override;

    /**
     * @brief Clear the entire history.
     */
    void clear();

    /**
     * @brief Check if undo is available.
     *
     * @return true if there are commands to undo.
     */
    [[nodiscard]] bool can_undo() const;

    /**
     * @brief Check if redo is available.
     *
     * @return true if there are commands to redo.
     */
    [[nodiscard]] bool can_redo() const;

    /**
     * @brief Get the number of undoable commands.
     *
     * @return The size of the undo stack.
     */
    [[nodiscard]] size_t undo_size() const;

    /**
     * @brief Get the number of redoable commands.
     *
     * @return The size of the redo stack.
     */
    [[nodiscard]] size_t redo_size() const;

  private:
    std::shared_ptr<HistoryStack> stack_;
};
}  // namespace gimp
