#pragma once

#include "core/selection_manager.h"
#include "core/tool.h"

#include <QPainterPath>
#include <QPoint>

namespace gimp {

/**
 * @brief Ellipse selection tool with modifier support.
 */
class EllipseSelectTool : public Tool {
  public:
    EllipseSelectTool() = default;

    [[nodiscard]] std::string id() const override { return "select_ellipse"; }
    [[nodiscard]] std::string name() const override { return "Ellipse Select"; }

  protected:
    void beginStroke(const ToolInputEvent& event) override;
    void continueStroke(const ToolInputEvent& event) override;
    void endStroke(const ToolInputEvent& event) override;
    void cancelStroke() override;

  private:
    static SelectionMode resolveSelectionMode(Qt::KeyboardModifiers modifiers,
                                              Qt::MouseButtons buttons);
    QPainterPath buildEllipsePath(const QPoint& start,
                                  const QPoint& current,
                                  Qt::KeyboardModifiers modifiers) const;

    QPoint startPos_;
    QPoint currentPos_;
    SelectionMode currentMode_ = SelectionMode::Replace;
};

}  // namespace gimp
