/**
 * @file pencil_tool.cpp
 * @brief Implementation of PencilTool.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#include "core/tools/pencil_tool.h"
#include "core/document.h"
#include "core/command_bus.h"
#include "core/commands/draw_command.h"

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

    if (!document_ || !commandBus_ || strokePoints_.empty()) {
        strokePoints_.clear();
        return;
    }

    // Find affected region bounds
    int minX = INT_MAX, maxX = INT_MIN;
    int minY = INT_MAX, maxY = INT_MIN;

    for (const auto& pt : strokePoints_) {
        int radius = brushSize_ / 2;
        minX = std::min(minX, pt.x - radius);
        maxX = std::max(maxX, pt.x + radius);
        minY = std::min(minY, pt.y - radius);
        maxY = std::max(maxY, pt.y + radius);
    }

    if (minX > maxX || minY > maxY) {
        strokePoints_.clear();
        return;
    }

    int width = maxX - minX + 1;
    int height = maxY - minY + 1;

    // Create command and capture before state
    auto drawCmd = std::make_shared<DrawCommand>(
        document_->layers()[0],  // TODO: Use active layer
        minX, minY, width, height);
    drawCmd->captureBeforeState();

    // TODO: Render the stroke to the layer using BrushStrategy

    // Capture after state and dispatch
    drawCmd->captureAfterState();
    commandBus_->dispatch(drawCmd);

    strokePoints_.clear();
}

void PencilTool::cancelStroke()
{
    strokePoints_.clear();
}

}  // namespace gimp
