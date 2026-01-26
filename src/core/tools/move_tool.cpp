/**
 * @file move_tool.cpp
 * @brief Implementation of MoveTool.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#include "core/tools/move_tool.h"

namespace gimp {

void MoveTool::beginStroke(const ToolInputEvent& event)
{
    startPos_ = event.canvasPos;
    currentPos_ = event.canvasPos;
}

void MoveTool::continueStroke(const ToolInputEvent& event)
{
    currentPos_ = event.canvasPos;
}

void MoveTool::endStroke(const ToolInputEvent& event)
{
    currentPos_ = event.canvasPos;
    lastDelta_ = currentPos_ - startPos_;

    // TODO: Issue MoveCommand to CommandBus when implemented.
    // The command should:
    // 1. Store the original layer position
    // 2. Apply the translation
    // 3. Support undo by restoring original position
}

void MoveTool::cancelStroke()
{
    lastDelta_ = QPoint(0, 0);
}

}  // namespace gimp
