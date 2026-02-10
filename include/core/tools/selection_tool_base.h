/**
 * @file selection_tool_base.h
 * @brief Base class for selection tools with common functionality.
 * @author Laurent Jiang
 * @date 2026-02-10
 */

#pragma once

#include "core/selection_manager.h"
#include "core/tool.h"

#include <memory>
#include <string>

namespace gimp {
class SelectionCommand;
}  // namespace gimp

namespace gimp {

/**
 * @brief Base class for selection tools.
 *
 * Provides common functionality shared by all selection tools:
 * - Selection mode resolution from modifiers (Ctrl=Add, Ctrl+Alt=Subtract)
 * - Command lifecycle management for undo/redo support
 * - Preview management during interactive selection
 *
 * Concrete selection tools should inherit from this and implement
 * the shape-specific path building logic.
 */
class SelectionToolBase : public Tool {
  public:
    ~SelectionToolBase() override = default;

  protected:
    SelectionToolBase() = default;

    /**
     * @brief Resolves selection mode from keyboard modifiers.
     *
     * - No modifier: Replace
     * - Ctrl: Add to selection
     * - Ctrl+Alt: Subtract from selection
     *
     * @param modifiers Active keyboard modifiers.
     * @return The selection mode to use.
     */
    static SelectionMode resolveSelectionMode(Qt::KeyboardModifiers modifiers);

    /**
     * @brief Creates and initializes a new selection command.
     *
     * Captures the before state for undo support. Call this at the start
     * of a selection operation (beginStroke or when entering adjust mode).
     *
     * @param description Human-readable description for undo history.
     */
    void beginSelectionCommand(const std::string& description);

    /**
     * @brief Finalizes and dispatches the pending selection command.
     *
     * Captures the after state and dispatches to the command bus.
     * Call this when a selection operation completes (endStroke or finalize).
     */
    void commitSelectionCommand();

    /**
     * @brief Clears preview and resets pending command.
     *
     * Called by cancelStroke implementations. Does not affect any
     * committed selection state.
     */
    void cancelSelectionOperation();

    /// Current selection combination mode for the active operation.
    SelectionMode currentMode_ = SelectionMode::Replace;

    /// Pending command for the current selection operation.
    std::shared_ptr<SelectionCommand> pendingCommand_;
};

}  // namespace gimp
