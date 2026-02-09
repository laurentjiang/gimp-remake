/**
 * @file move_command.h
 * @brief Command to move selection contents (undoable).
 * @author Laurent Jiang
 * @date 2026-02-09
 */

#pragma once

#include "core/command.h"

#include <QPainterPath>
#include <QRect>

#include <cstdint>
#include <memory>
#include <vector>

namespace gimp {
class Layer;

/**
 * @class MoveCommand
 * @brief Command to move selection contents with undo support.
 *
 * Captures the affected region before and after the move to support undo/redo.
 * The affected region is the union of the source and destination areas.
 */
class MoveCommand : public Command {
  public:
    /**
     * @brief Constructs a move command for a specific layer.
     * @param layer The target layer.
     * @param affectedRegion The bounding rectangle of all pixels that will change.
     */
    MoveCommand(std::shared_ptr<Layer> layer, QRect affectedRegion);

    ~MoveCommand() override = default;

    /**
     * @brief Applies the move operation by restoring the "after" state.
     */
    void apply() override;

    /**
     * @brief Undoes the move operation by restoring the "before" state.
     */
    void undo() override;

    /**
     * @brief Captures the current state of the affected region (before state).
     *
     * Call this before performing the move operation.
     */
    void captureBeforeState();

    /**
     * @brief Captures the current state of the affected region (after state).
     *
     * Call this after performing the move operation.
     */
    void captureAfterState();

  private:
    std::shared_ptr<Layer> layer_;
    QRect affectedRegion_;                   ///< Bounding box of all changed pixels.
    std::vector<std::uint8_t> beforeState_;  ///< Pixel data before move.
    std::vector<std::uint8_t> afterState_;   ///< Pixel data after move.

    /**
     * @brief Updates pixel data from a saved state.
     * @param state The state buffer to restore from.
     */
    void updateState(const std::vector<std::uint8_t>& state);
};

}  // namespace gimp
