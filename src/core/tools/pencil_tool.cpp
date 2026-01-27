/**
 * @file pencil_tool.cpp
 * @brief Implementation of PencilTool.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#include "core/tools/pencil_tool.h"

#include "core/brush_strategy.h"
#include "core/command_bus.h"
#include "core/commands/draw_command.h"
#include "core/document.h"
#include "core/layer.h"

#include <cmath>

namespace gimp {

namespace {

/**
 * @brief Interpolates points along a line between two stroke points.
 *
 * Uses linear interpolation to ensure smooth, continuous strokes without gaps.
 *
 * @param from Starting point.
 * @param to Ending point.
 * @param brushSize Spacing is approximately half the brush size.
 * @return Vector of interpolated points including endpoints.
 */
std::vector<std::tuple<int, int, float>> interpolatePoints(int fromX,
                                                           int fromY,
                                                           float fromPressure,
                                                           int toX,
                                                           int toY,
                                                           float toPressure,
                                                           int brushSize)
{
    std::vector<std::tuple<int, int, float>> result;

    int dx = toX - fromX;
    int dy = toY - fromY;
    float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));

    // Spacing: at most half the brush size for smooth strokes
    float spacing = std::max(1.0F, static_cast<float>(brushSize) / 4.0F);
    int steps = std::max(1, static_cast<int>(distance / spacing));

    for (int i = 0; i <= steps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        int x = fromX + static_cast<int>(static_cast<float>(dx) * t);
        int y = fromY + static_cast<int>(static_cast<float>(dy) * t);
        float pressure = fromPressure + (toPressure - fromPressure) * t;
        result.emplace_back(x, y, pressure);
    }

    return result;
}

}  // namespace

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

    // Get the target layer and render the stroke
    auto layer = document_->layers()[0];
    auto* pixelData = layer->data().data();
    int layerWidth = layer->width();
    int layerHeight = layer->height();

    SolidBrush brush;

    // Render dabs along the stroke path with interpolation
    for (size_t i = 0; i < strokePoints_.size(); ++i) {
        if (i == 0) {
            // First point: render single dab
            brush.renderDab(pixelData,
                            layerWidth,
                            layerHeight,
                            strokePoints_[i].x,
                            strokePoints_[i].y,
                            brushSize_,
                            color_,
                            strokePoints_[i].pressure);
        } else {
            // Interpolate between previous and current point
            auto interpolated = interpolatePoints(strokePoints_[i - 1].x,
                                                  strokePoints_[i - 1].y,
                                                  strokePoints_[i - 1].pressure,
                                                  strokePoints_[i].x,
                                                  strokePoints_[i].y,
                                                  strokePoints_[i].pressure,
                                                  brushSize_);

            // Skip first point to avoid double-rendering
            for (size_t j = 1; j < interpolated.size(); ++j) {
                auto [x, y, pressure] = interpolated[j];
                brush.renderDab(
                    pixelData, layerWidth, layerHeight, x, y, brushSize_, color_, pressure);
            }
        }
    }

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
