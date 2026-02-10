/**
 * @file selection_command.h
 * @brief Command for undoable selection operations.
 * @author Laurent Jiang
 * @date 2026-02-09
 */

#pragma once

#include "core/command.h"
#include "core/selection_manager.h"

#include <QPainterPath>

#include <string>

namespace gimp {

/**
 * @brief Command to modify selection with undo/redo support.
 *
 * Stores the selection path before and after the operation to enable
 * reversible selection changes.
 */
class SelectionCommand : public Command {
  public:
    /**
     * @brief Constructs a selection command.
     * @param description Human-readable description of the operation.
     */
    explicit SelectionCommand(std::string description);

    ~SelectionCommand() override = default;

    /**
     * @brief Applies the selection change (restores after state).
     */
    void apply() override;

    /**
     * @brief Undoes the selection change (restores before state).
     */
    void undo() override;

    /**
     * @brief Captures the current selection as the "before" state.
     *
     * Call this before making the selection change.
     */
    void captureBeforeState();

    /**
     * @brief Captures the current selection as the "after" state.
     *
     * Call this after making the selection change.
     */
    void captureAfterState();

    /**
     * @brief Returns the description of this command.
     * @return Command description string.
     */
    [[nodiscard]] const std::string& description() const { return description_; }

  private:
    std::string description_;
    QPainterPath beforePath_;
    QPainterPath afterPath_;
    SelectionType beforeType_ = SelectionType::Unknown;  ///< Selection type before.
    SelectionType afterType_ = SelectionType::Unknown;   ///< Selection type after.
};

}  // namespace gimp
