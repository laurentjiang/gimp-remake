/**
 * @file pencil_tool.cpp
 * @brief Implementation of PencilTool.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#include "core/tools/pencil_tool.h"

namespace gimp {

void PencilTool::beginStroke(const ToolInputEvent& event)
{
    strokePoints_.clear();
    strokePoints_.push_back({event.canvasPos.x(), event.canvasPos.y(), event.pressure});
}

void PencilTool::continueStroke(const ToolInputEvent& event)
{
    strokePoints_.push_back({event.canvasPos.x(), event.canvasPos.y(), event.pressure});
}

void PencilTool::endStroke(const ToolInputEvent& event)
{
    strokePoints_.push_back({event.canvasPos.x(), event.canvasPos.y(), event.pressure});

    // TODO: Issue DrawCommand to CommandBus when #26 is implemented.
    // The command should:
    // 1. Capture the "before" state of affected pixels
    // 2. Render the stroke to the active layer
    // 3. Store the "after" state for redo

    strokePoints_.clear();
}

void PencilTool::cancelStroke()
{
    strokePoints_.clear();
}

}  // namespace gimp
