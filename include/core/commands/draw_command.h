/**
 * @file draw_command.h
 * @brief Command to draw strokes on a layer (undoable).
 * @author Aless Tosi
 * @date 2026-01-25
 */

#pragma once

#include "core/command.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace gimp {
class Layer;

/*!
 * @class DrawCommand
 * @brief Command to draw a stroke on a layer with undo support.
 *
 * Captures the affected region before and after drawing to support undo/redo.
 */
class DrawCommand : public Command {
  public:
    /*!
     * @brief Constructs a draw command for a specific layer.
     * @param layer The target layer to draw on.
     * @param x Left edge of the affected region.
     * @param y Top edge of the affected region.
     * @param width Width of the affected region in pixels.
     * @param height Height of the affected region in pixels.
     */
    DrawCommand(std::shared_ptr<Layer> layer, int x, int y, int width, int height);

    ~DrawCommand() override = default;

    /*!
     * @brief Applies the draw operation by restoring the "after" state.
     */
    void apply() override;

    /*!
     * @brief Undoes the draw operation by restoring the "before" state.
     */
    void undo() override;

    /*!
     * @brief Captures the current state of the affected region (before state).
     *
     * Call this before performing the draw operation.
     */
    void captureBeforeState();

    /*!
     * @brief Captures the current state of the affected region (after state).
     *
     * Call this after performing the draw operation.
     */
    void captureAfterState();

  private:
    std::shared_ptr<Layer> layer_;
    int regionX_;                            ///< Left edge of affected region.
    int regionY_;                            ///< Top edge of affected region.
    int regionWidth_;                        ///< Width of affected region.
    int regionHeight_;                       ///< Height of affected region.
    std::vector<std::uint8_t> beforeState_;  ///< Pixel data before drawing.
    std::vector<std::uint8_t> afterState_;   ///< Pixel data after drawing.

    /*!
     * @brief Updates pixel data from a saved state.
     * @param state The state buffer to restore from.
     */
    void updateState(const std::vector<std::uint8_t>& state);
};
}  // namespace gimp
