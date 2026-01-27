/**
 * @file pencil_tool.cpp
 * @brief Implementation of PencilTool.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#include "core/tools/pencil_tool.h"

#include "core/command_bus.h"
#include "core/commands/draw_command.h"
#include "core/document.h"

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

std::shared_ptr<DrawCommand> PencilTool::buildDrawCommand(int minX, int maxX, int minY, int maxY)
{
    for (const auto& pt : strokePoints_) {
        int radius = brushSize_ / 2;
        minX = std::min(minX, pt.x - radius);
        maxX = std::max(maxX, pt.x + radius);
        minY = std::min(minY, pt.y - radius);
        maxY = std::max(maxY, pt.y + radius);
    }

    if (minX > maxX || minY > maxY) {
        strokePoints_.clear();
        return nullptr;
    }

    int width = maxX - minX + 1;
    int height = maxY - minY + 1;

    return std::make_shared<DrawCommand>(document_->layers()[0], minX, minY, width, height);
}

void PencilTool::endStroke(const ToolInputEvent& event)
{
    strokePoints_.push_back({event.canvasPos.x(), event.canvasPos.y(), event.pressure});

    if (!document_ || !commandBus_ || strokePoints_.empty()) {
        strokePoints_.clear();
        return;
    }

    // Create command
    auto drawCmd = buildDrawCommand(INT_MAX, INT_MIN, INT_MAX, INT_MIN);
    if (!drawCmd) {
        return;
    }

    // Capture before state
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
