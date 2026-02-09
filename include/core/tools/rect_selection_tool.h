/**
 * @file rect_selection_tool.h
 * @brief Rectangle selection tool with modifier support.
 * @author Aless Tosi
 * @date 2026-02-09
 */

#pragma once

#include "core/selection_manager.h"
#include "core/tool.h"

#include <QPainterPath>
#include <QPoint>
#include <QRectF>

#include <memory>
#include <vector>

namespace gimp {
class SelectionCommand;
}  // namespace gimp

namespace gimp {

/**
 * @brief Selection tool phase.
 */
enum class SelectionPhase {
    Idle,      ///< No active selection operation
    Creating,  ///< Drawing a new selection
    Adjusting  ///< Resizing an existing selection via handles
};

/**
 * @brief Handle position for selection resize.
 */
enum class SelectionHandle {
    None,
    TopLeft,
    Top,
    TopRight,
    Right,
    BottomRight,
    Bottom,
    BottomLeft,
    Left
};

/**
 * @brief Rectangle selection tool with modifier support.
 *
 * After creating a selection, handles appear around it. The user can drag
 * handles to resize the selection outline (marching ants). Press Enter to
 * finalize, Escape to cancel, or click outside to start a new selection.
 */
class RectSelectTool : public Tool {
  public:
    RectSelectTool() = default;

    [[nodiscard]] std::string id() const override { return "select_rect"; }
    [[nodiscard]] std::string name() const override { return "Rectangle Select"; }

    /**
     * @brief Returns the current selection phase.
     */
    [[nodiscard]] SelectionPhase phase() const { return phase_; }

    /**
     * @brief Returns handle rectangles for the current selection bounds.
     * @param zoomLevel Current canvas zoom for screen-space sizing.
     * @return Vector of 8 handle rects in canvas coordinates.
     */
    [[nodiscard]] std::vector<QRectF> getHandleRects(float zoomLevel) const;

    /**
     * @brief Handles key events for Enter/Escape.
     */
    bool onKeyPress(Qt::Key key, Qt::KeyboardModifiers modifiers) override;

    /**
     * @brief Called when tool is deactivated. Resets phase state.
     */
    void onDeactivate() override;

    /**
     * @brief Resets tool to idle state, clearing phase and bounds.
     * Can be called externally when selection is taken over by another tool.
     */
    void resetToIdle();

  protected:
    void beginStroke(const ToolInputEvent& event) override;
    void continueStroke(const ToolInputEvent& event) override;
    void endStroke(const ToolInputEvent& event) override;
    void cancelStroke() override;

  private:
    static SelectionMode resolveSelectionMode(Qt::KeyboardModifiers modifiers);
    QPainterPath buildRectPath(const QRectF& rect) const;
    SelectionHandle hitTestHandle(const QPoint& pos, float zoomLevel) const;
    QPointF getAnchorForHandle(SelectionHandle handle) const;
    void finalizeSelection();

    SelectionPhase phase_ = SelectionPhase::Idle;
    QRectF currentBounds_;  ///< Current selection bounding rect.
    QPoint startPos_;       ///< Start position for creating or drag start.
    SelectionMode currentMode_ = SelectionMode::Replace;
    SelectionHandle activeHandle_ = SelectionHandle::None;
    QPointF scaleAnchor_;  ///< Fixed corner during resize.

    /// Pending command for current selection operation.
    std::shared_ptr<SelectionCommand> pendingCommand_;

    /// Creates and initializes a new selection command.
    void beginSelectionCommand(const std::string& description);

    /// Finalizes and dispatches the pending command.
    void commitSelectionCommand();
};

}  // namespace gimp
