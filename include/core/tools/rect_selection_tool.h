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

namespace gimp {

/**
 * @brief Rectangle selection tool with modifier support.
 */
class RectSelectTool : public Tool {
  public:
    RectSelectTool() = default;

    [[nodiscard]] std::string id() const override { return "select_rect"; }
    [[nodiscard]] std::string name() const override { return "Rectangle Select"; }

  protected:
    void beginStroke(const ToolInputEvent& event) override;
    void continueStroke(const ToolInputEvent& event) override;
    void endStroke(const ToolInputEvent& event) override;
    void cancelStroke() override;

  private:
    static SelectionMode resolveSelectionMode(Qt::KeyboardModifiers modifiers);
    QPainterPath buildRectPath(const QPoint& start,
                               const QPoint& current,
                               Qt::KeyboardModifiers modifiers) const;

    QPoint startPos_;
    QPoint currentPos_;
    SelectionMode currentMode_ = SelectionMode::Replace;
};

}  // namespace gimp
