#pragma once

#include "core/selection_manager.h"
#include "core/tool.h"

#include <QPainterPath>
#include <QPoint>
#include <QRectF>

#include <array>
#include <memory>

namespace gimp {
class SelectionCommand;
}  // namespace gimp

namespace gimp {

/// Phase state machine for ellipse selection tool
enum class EllipseSelectionPhase {
    Idle,
    Creating,
    Adjusting
};

/// Handle identifiers for ellipse selection resize
enum class EllipseSelectionHandle {
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

/// Screen-space handle size (pixels) - constant regardless of zoom
inline constexpr float kEllipseHandleScreenSize = 8.0F;

/**
 * @brief Ellipse selection tool with modifier support and handle-based resize.
 */
class EllipseSelectTool : public Tool {
  public:
    EllipseSelectTool() = default;

    [[nodiscard]] std::string id() const override { return "select_ellipse"; }
    [[nodiscard]] std::string name() const override { return "Ellipse Select"; }

    /// Current selection phase
    [[nodiscard]] EllipseSelectionPhase phase() const { return phase_; }

    /// Get handle rects in canvas coordinates (zoomLevel needed for screen-space sizing)
    [[nodiscard]] std::array<QRectF, 8> getHandleRects(float zoomLevel) const;

    /// Handle key press for Enter/Escape during Adjusting phase
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
    QPainterPath buildEllipsePath(const QPoint& start,
                                  const QPoint& current,
                                  Qt::KeyboardModifiers modifiers) const;

    /// Hit test for handles - returns handle index or None
    [[nodiscard]] EllipseSelectionHandle hitTestHandle(const QPointF& pos, float zoomLevel) const;

    /// Get anchor point for a given handle (opposite corner/edge)
    [[nodiscard]] QPointF getAnchorForHandle(EllipseSelectionHandle handle) const;

    QPoint startPos_;
    QPoint currentPos_;
    SelectionMode currentMode_ = SelectionMode::Replace;

    // Phase state
    EllipseSelectionPhase phase_ = EllipseSelectionPhase::Idle;
    QRectF currentBounds_;  // Current selection bounding rect

    // Handle resize state
    EllipseSelectionHandle activeHandle_ = EllipseSelectionHandle::None;
    QPointF scaleAnchor_;    // Fixed anchor point during resize
    QRectF originalBounds_;  // Bounds before resize started

    /// Pending command for current selection operation.
    std::shared_ptr<SelectionCommand> pendingCommand_;

    /// Creates and initializes a new selection command.
    void beginSelectionCommand(const std::string& description);

    /// Finalizes and dispatches the pending command.
    void commitSelectionCommand();
};

}  // namespace gimp
