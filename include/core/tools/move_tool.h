/**
 * @file move_tool.h
 * @brief Move tool for translating layers or selections.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#pragma once

#include "core/floating_buffer.h"
#include "core/tool.h"
#include "core/tool_options.h"
#include "core/transform_state.h"

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
    [[nodiscard]] bool isMovingSelection() const { return !buffer_.isEmpty(); }

    /*! @brief Returns the floating buffer pixel data.
     *  @return Pointer to RGBA data, or nullptr if not moving selection.
     */
    [[nodiscard]] const std::vector<std::uint8_t>* floatingBuffer() const
    {
        return buffer_.isEmpty() ? nullptr : &buffer_.data();
    }

    /*! @brief Returns the floating buffer bounds (source rect).
     *  Used for pixel rendering position - may be clipped to layer bounds.
     *  @return Bounding rectangle of the extracted pixels.
     */
    [[nodiscard]] QRect floatingRect() const { return buffer_.sourceRect(); }

    /*! @brief Returns the full selection bounds for visual elements.
     *  Used for marching ants and handles - not clipped to layer bounds.
     *  @return Original selection bounding rectangle.
     */
    [[nodiscard]] QRectF selectionBounds() const { return transform_.originalBounds(); }

    /*! @brief Returns the current movement offset for the floating buffer.
     *  @return Offset from original position.
     */
    [[nodiscard]] QPoint floatingOffset() const { return transform_.translation().toPoint(); }

    /*! @brief Returns the current scale factors for the floating buffer.
     *  @return Scale factor (1.0 = no scale).
     */
    [[nodiscard]] QSizeF floatingScale() const { return transform_.scale(); }

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
     * @param zoomLevel Current zoom level for size compensation.
     * @return Vector of handle rects (order matches TransformHandle enum, excluding None).
     */
    [[nodiscard]] std::vector<QRect> getHandleRects(float zoomLevel = 1.0F) const;

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

    /**
     * @brief Cancels the current floating buffer (if any) without committing.
     *
     * This is used when an external action (like undo) needs to discard
     * a pending move operation. Pixels are restored to their original
     * position and the floating buffer is cleared.
     */
    void cancelFloatingBuffer();

    /**
     * @brief Hit-tests for a transform handle at the given position.
     * @param pos Canvas position to test.
     * @param zoomLevel Current zoom level for size compensation.
     * @return The handle at that position, or None.
     */
    [[nodiscard]] TransformHandle hitTestHandle(const QPoint& pos, float zoomLevel = 1.0F) const;

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
    bool proportionalScale_ = false;  ///< True if Shift is held for proportional scaling.

    // Floating buffer and transform using new encapsulated classes
    FloatingBuffer buffer_;               ///< Extracted pixels and selection mask.
    TransformState transform_;            ///< Current transformation state.
    std::shared_ptr<Layer> targetLayer_;  ///< Layer being modified.

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
};

}  // namespace gimp
