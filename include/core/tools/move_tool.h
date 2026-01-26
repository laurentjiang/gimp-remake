/**
 * @file move_tool.h
 * @brief Move tool for translating layers or selections.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#pragma once

#include "../tool.h"

#include <QPoint>

namespace gimp {

/**
 * @brief Tool for moving layers or selections.
 *
 * The move tool translates the active layer or selection by dragging.
 * It records the delta movement and issues a MoveCommand on commit.
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

  protected:
    void beginStroke(const ToolInputEvent& event) override;
    void continueStroke(const ToolInputEvent& event) override;
    void endStroke(const ToolInputEvent& event) override;
    void cancelStroke() override;

  private:
    QPoint startPos_;      ///< Initial mouse position.
    QPoint currentPos_;    ///< Current mouse position.
    QPoint lastDelta_;     ///< Recorded movement from last completed stroke.
};

}  // namespace gimp
