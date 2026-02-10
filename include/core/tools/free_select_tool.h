/**
 * @file free_select_tool.h
 * @brief Free-form (lasso) selection tool.
 * @author Laurent Jiang
 * @date 2026-02-09
 */

#pragma once

#include "core/selection_manager.h"
#include "core/tool.h"

#include <QPainterPath>
#include <QPointF>

#include <vector>

namespace gimp {

/**
 * @brief Free-form selection tool allowing arbitrary polygon selection.
 *
 * Users draw a freehand path by clicking and dragging. On release,
 * the path is closed and applied to the selection. Supports add/subtract
 * modes via Ctrl/Alt modifiers.
 */
class FreeSelectTool : public Tool {
  public:
    FreeSelectTool() = default;

    [[nodiscard]] std::string id() const override { return "select_free"; }
    [[nodiscard]] std::string name() const override { return "Free Select"; }

  protected:
    void beginStroke(const ToolInputEvent& event) override;
    void continueStroke(const ToolInputEvent& event) override;
    void endStroke(const ToolInputEvent& event) override;
    void cancelStroke() override;

  private:
    /**
     * @brief Resolves the selection mode from input modifiers and buttons.
     * @param modifiers Active keyboard modifiers.
     * @param buttons Active mouse buttons.
     * @return Selection mode (Replace, Add, or Subtract).
     */
    static SelectionMode resolveSelectionMode(Qt::KeyboardModifiers modifiers,
                                              Qt::MouseButtons buttons);

    /**
     * @brief Builds a closed painter path from collected points.
     * @param close Whether to close the path (for final selection).
     * @return The constructed QPainterPath.
     */
    [[nodiscard]] QPainterPath buildPath(bool close) const;

    std::vector<QPointF> points_;
    SelectionMode currentMode_ = SelectionMode::Replace;

    /// Minimum distance between points to avoid excessive density.
    static constexpr float kMinPointDistance = 2.0F;
};

}  // namespace gimp
