/**
 * @file move_tool.h
 * @brief Move tool for translating layers or selections.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#pragma once

#include "core/tool.h"

#include <QPainterPath>
#include <QPoint>
#include <QRect>

#include <cstdint>
#include <memory>
#include <vector>

namespace gimp {

class Layer;

/**
 * @brief Tool for moving layers or selections.
 *
 * When a selection exists and the user clicks inside it, the selected pixels
 * are extracted into a floating buffer, the source is cleared to transparent,
 * and the buffer is rendered at the cursor offset during drag. On release or
 * Enter, the floating buffer is committed. Escape cancels the move.
 */
class MoveTool : public Tool {
  public:
    MoveTool() = default;

    [[nodiscard]] std::string id() const override { return "move"; }
    [[nodiscard]] std::string name() const override { return "Move"; }

    /*! @brief Returns the total movement delta from the last stroke.
     *  @return Movement delta in canvas coordinates.
     */
    [[nodiscard]] QPoint lastDelta() const { return lastDelta_; }

    /*! @brief Returns true if currently moving a selection.
     *  @return True if floating buffer is active.
     */
    [[nodiscard]] bool isMovingSelection() const { return !floatingBuffer_.empty(); }

    /*! @brief Returns the floating buffer pixel data.
     *  @return Pointer to RGBA data, or nullptr if not moving selection.
     */
    [[nodiscard]] const std::vector<std::uint8_t>* floatingBuffer() const
    {
        return floatingBuffer_.empty() ? nullptr : &floatingBuffer_;
    }

    /*! @brief Returns the floating buffer bounds (source rect).
     *  @return Bounding rectangle of the extracted pixels.
     */
    [[nodiscard]] QRect floatingRect() const { return floatingRect_; }

    /*! @brief Returns the current movement offset for the floating buffer.
     *  @return Offset from original position.
     */
    [[nodiscard]] QPoint floatingOffset() const { return currentPos_ - startPos_; }

    /**
     * @brief Handles key events during active move.
     * @param key The key pressed.
     * @param modifiers Active modifiers.
     * @return True if handled (Enter commits, Escape cancels).
     */
    bool onKeyPress(Qt::Key key, Qt::KeyboardModifiers modifiers) override;

  protected:
    void beginStroke(const ToolInputEvent& event) override;
    void continueStroke(const ToolInputEvent& event) override;
    void endStroke(const ToolInputEvent& event) override;
    void cancelStroke() override;

  private:
    QPoint startPos_;    ///< Initial mouse position.
    QPoint currentPos_;  ///< Current mouse position.
    QPoint lastDelta_;   ///< Recorded movement from last completed stroke.

    // Floating buffer for selection move
    std::vector<std::uint8_t> floatingBuffer_;  ///< Extracted pixels (RGBA).
    QRect floatingRect_;                        ///< Bounding rect of extracted region.
    std::shared_ptr<Layer> targetLayer_;        ///< Layer being modified.
    std::vector<bool> selectionMask_;           ///< Pre-rasterized selection mask.

    /**
     * @brief Pre-rasterizes the selection path into a boolean mask.
     * @param selPath The selection path to rasterize.
     * @param bounds The bounding rectangle for the mask.
     */
    void rasterizeSelectionMask(const QPainterPath& selPath, const QRect& bounds);

    /**
     * @brief Extracts pixels from the selection region into floatingBuffer_.
     * @param layer The layer to extract from.
     */
    void extractSelectionPixels(const std::shared_ptr<Layer>& layer);

    /**
     * @brief Clears the source pixels in the selection region to transparent.
     * @param layer The layer to modify.
     */
    void clearSourcePixels(const std::shared_ptr<Layer>& layer);

    /**
     * @brief Pastes the floating buffer at the new position.
     * @param layer The layer to paste to.
     * @param offset The movement offset from original position.
     */
    void pasteFloatingBuffer(const std::shared_ptr<Layer>& layer, QPoint offset);

    /**
     * @brief Commits the move operation with undo support.
     */
    void commitMove();

    /**
     * @brief Restores original pixels and cancels the move.
     */
    void cancelMove();

    /**
     * @brief Clears floating buffer state.
     */
    void clearFloatingState();

    /**
     * @brief Checks if a pixel in the mask is selected.
     * @param col Column in floatingRect_ coordinates.
     * @param row Row in floatingRect_ coordinates.
     * @return True if the pixel is inside the selection.
     */
    [[nodiscard]] bool isPixelSelected(int col, int row) const;
};

}  // namespace gimp
