/**
 * @file move_tool.h
 * @brief Move tool for translating layers or selections.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#pragma once

#include "core/tool.h"
#include "core/tool_options.h"

#include <QPainterPath>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QSizeF>

#include <cstdint>
#include <memory>
#include <vector>

namespace gimp {

class Layer;

/**
 * @brief Move operation mode.
 */
enum class MoveMode {
    Cut,  ///< Cut pixels from source (source becomes transparent)
    Copy  ///< Copy pixels (source remains intact)
};

/**
 * @brief Transform handle positions.
 */
enum class TransformHandle {
    None,         ///< No handle (move mode)
    TopLeft,      ///< Top-left corner
    Top,          ///< Top edge center
    TopRight,     ///< Top-right corner
    Right,        ///< Right edge center
    BottomRight,  ///< Bottom-right corner
    Bottom,       ///< Bottom edge center
    BottomLeft,   ///< Bottom-left corner
    Left          ///< Left edge center
};

/**
 * @brief Tool for moving layers or selections.
 *
 * When a selection exists and the user clicks inside it, the selected pixels
 * are extracted into a floating buffer, the source is cleared to transparent,
 * and the buffer is rendered at the cursor offset during drag. On release or
 * Enter, the floating buffer is committed. Escape cancels the move.
 */
class MoveTool : public Tool, public ToolOptions {
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

    /*! @brief Returns the current scale factors for the floating buffer.
     *  @return Scale factor (1.0 = no scale).
     */
    [[nodiscard]] QSizeF floatingScale() const { return currentScale_; }

    /*! @brief Returns true if currently scaling (vs just moving).
     *  @return True if a transform handle is being dragged.
     */
    [[nodiscard]] bool isScaling() const { return activeHandle_ != TransformHandle::None; }

    /*! @brief Returns the active transform handle.
     *  @return The handle being dragged, or None.
     */
    [[nodiscard]] TransformHandle activeHandle() const { return activeHandle_; }

    /**
     * @brief Returns the 8 handle rectangles in canvas coordinates.
     * @return Vector of handle rects (order matches TransformHandle enum, excluding None).
     */
    [[nodiscard]] std::vector<QRect> getHandleRects() const;

    /**
     * @brief Scales the floating buffer to the current scale factors.
     * @return Scaled pixel data.
     */
    [[nodiscard]] std::vector<std::uint8_t> getScaledBuffer() const;

    /**
     * @brief Returns the scaled size of the floating buffer.
     * @return Scaled dimensions.
     */
    [[nodiscard]] QSize getScaledSize() const;

    /**
     * @brief Handles key events during active move.
     * @param key The key pressed.
     * @param modifiers Active modifiers.
     * @return True if handled (Enter commits, Escape cancels).
     */
    bool onKeyPress(Qt::Key key, Qt::KeyboardModifiers modifiers) override;

    /**
     * @brief Sets copy mode for the next stroke (modifier override).
     *
     * When copy mode is enabled, the source pixels are NOT cleared,
     * resulting in a copy-move operation (like Shift+Alt in GIMP).
     * This temporarily overrides the UI setting for the current stroke.
     *
     * @param copyMode True to copy pixels, false to cut them.
     */
    void setCopyMode(bool copyMode)
    {
        modifierOverride_ = true;
        modifierCopyMode_ = copyMode;
    }

    /**
     * @brief Commits the current floating buffer (if any) to the layer.
     *
     * This is used when an external action (like starting a new tool stroke)
     * needs to finalize a pending move operation. Pixels are pasted at the
     * current offset and the selection path is updated.
     */
    void commitFloatingBuffer();

    // ToolOptions interface
    [[nodiscard]] std::vector<ToolOption> getOptions() const override;
    void setOptionValue(const std::string& optionId,
                        const std::variant<int, float, bool, std::string>& value) override;
    [[nodiscard]] std::variant<int, float, bool, std::string> getOptionValue(
        const std::string& optionId) const override;

  protected:
    void beginStroke(const ToolInputEvent& event) override;
    void continueStroke(const ToolInputEvent& event) override;
    void endStroke(const ToolInputEvent& event) override;
    void cancelStroke() override;

  private:
    QPoint startPos_;                    ///< Initial mouse position.
    QPoint currentPos_;                  ///< Current mouse position.
    QPoint lastDelta_;                   ///< Recorded movement from last completed stroke.
    MoveMode moveMode_ = MoveMode::Cut;  ///< Default move mode from tool options.
    bool modifierOverride_ = false;      ///< True if modifier key overrode the UI setting.
    bool modifierCopyMode_ = false;      ///< Copy mode from modifier override.

    // Transform state
    TransformHandle activeHandle_ = TransformHandle::None;  ///< Handle being dragged.
    QSizeF currentScale_{1.0, 1.0};   ///< Current scale factors during transform.
    QSizeF originalSize_;             ///< Original floating buffer size.
    QPointF scaleAnchor_;             ///< Anchor point (opposite corner) for scaling.
    bool proportionalScale_ = false;  ///< True if Shift is held for proportional scaling.

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
     * @brief Checks if the destination rect is fully inside layer bounds.
     * @return True if all pixels would fit inside the layer.
     */
    [[nodiscard]] bool isDestinationInsideBounds() const;

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

    /**
     * @brief Hit-tests for a transform handle at the given position.
     * @param pos Canvas position to test.
     * @return The handle at that position, or None.
     */
    [[nodiscard]] TransformHandle hitTestHandle(const QPoint& pos) const;

    /**
     * @brief Returns the anchor point for the given handle (opposite corner/edge).
     * @param handle The handle being dragged.
     * @param bounds The bounding rect.
     * @return Anchor point in canvas coordinates.
     */
    [[nodiscard]] static QPointF getAnchorForHandle(TransformHandle handle, const QRectF& bounds);
};

}  // namespace gimp
