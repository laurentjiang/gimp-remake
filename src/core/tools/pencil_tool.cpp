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
#include "core/tool_factory.h"

#include <cmath>

namespace gimp {

namespace {

/**
 * @brief Interpolates points along a line between two stroke points.
 *
 * Uses linear interpolation to ensure smooth, continuous strokes without gaps.
 *
 * @param fromX Starting X position.
 * @param fromY Starting Y position.
 * @param fromPressure Starting pressure.
 * @param toX Ending X position.
 * @param toY Ending Y position.
 * @param toPressure Ending pressure.
 * @param brushSize Spacing is approximately 1/4 the brush size.
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

    // Spacing: at most 1/4 the brush size for smooth strokes
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

void PencilTool::renderSegment(int fromX,
                               int fromY,
                               float fromPressure,
                               int toX,
                               int toY,
                               float toPressure)
{
    if (!document_ || document_->layers().count() == 0) {
        return;
    }

    auto layer = document_->layers()[0];
    auto* pixelData = layer->data().data();
    int layerWidth = layer->width();
    int layerHeight = layer->height();

    SolidBrush brush;
    std::uint32_t color = ToolFactory::instance().foregroundColor();

    auto interpolated =
        interpolatePoints(fromX, fromY, fromPressure, toX, toY, toPressure, brushSize_);

    for (const auto& [x, y, pressure] : interpolated) {
        // Pencil tool ignores pressure for consistent hard-edged strokes
        (void)pressure;
        brush.renderDab(pixelData, layerWidth, layerHeight, x, y, brushSize_, color, 1.0F);
    }
}

void PencilTool::beginStroke(const ToolInputEvent& event)
{
    strokePoints_.clear();
    beforeState_.clear();

    if (!document_ || document_->layers().count() == 0) {
        return;
    }

    // Capture the layer state before we start drawing
    auto layer = document_->layers()[0];
    beforeState_ = layer->data();

    // Add first point and render it
    strokePoints_.push_back({event.canvasPos.x(), event.canvasPos.y(), event.pressure});

    auto* pixelData = layer->data().data();
    int layerWidth = layer->width();
    int layerHeight = layer->height();

    SolidBrush brush;
    std::uint32_t color = ToolFactory::instance().foregroundColor();
    // Pencil tool ignores pressure for consistent hard-edged strokes
    brush.renderDab(pixelData,
                    layerWidth,
                    layerHeight,
                    event.canvasPos.x(),
                    event.canvasPos.y(),
                    brushSize_,
                    color,
                    1.0F);
}

void PencilTool::continueStroke(const ToolInputEvent& event)
{
    if (strokePoints_.empty()) {
        return;
    }

    const auto& lastPoint = strokePoints_.back();
    int newX = event.canvasPos.x();
    int newY = event.canvasPos.y();

    // Only render if the mouse moved
    if (newX != lastPoint.x || newY != lastPoint.y) {
        renderSegment(lastPoint.x, lastPoint.y, lastPoint.pressure, newX, newY, event.pressure);
        strokePoints_.push_back({newX, newY, event.pressure});
    }
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
    if (strokePoints_.empty() || beforeState_.empty()) {
        strokePoints_.clear();
        beforeState_.clear();
        return;
    }

    // Render the final segment
    const auto& lastPoint = strokePoints_.back();
    int newX = event.canvasPos.x();
    int newY = event.canvasPos.y();
    if (newX != lastPoint.x || newY != lastPoint.y) {
        renderSegment(lastPoint.x, lastPoint.y, lastPoint.pressure, newX, newY, event.pressure);
        strokePoints_.push_back({newX, newY, event.pressure});
    }

    if (!document_ || !commandBus_) {
        strokePoints_.clear();
        beforeState_.clear();
        return;
    }

    // Create command for the affected region
    auto drawCmd = buildDrawCommand(INT_MAX, INT_MIN, INT_MAX, INT_MIN);
    if (!drawCmd) {
        beforeState_.clear();
        return;
    }

    // The layer now has the "after" state (with the stroke)
    // We need to swap in the "before" state, capture it, then swap back
    auto layer = document_->layers()[0];
    std::vector<uint8_t> afterState = layer->data();

    // Temporarily restore before state
    layer->data() = beforeState_;
    drawCmd->captureBeforeState();

    // Restore after state (the drawn stroke)
    layer->data() = afterState;
    drawCmd->captureAfterState();

    commandBus_->dispatch(drawCmd);

    // Mark the foreground color as used for recent colors tracking
    ToolFactory::instance().markForegroundColorUsed();

    strokePoints_.clear();
    beforeState_.clear();
}

void PencilTool::cancelStroke()
{
    strokePoints_.clear();
}

std::vector<ToolOption> PencilTool::getOptions() const
{
    return {
        ToolOption{
                   "brush_size", "Brush Size", ToolOption::Type::Slider, brushSize_, 1.0F, 100.0F, 1.0F},
        ToolOption{"opacity",
                   "Opacity",                  ToolOption::Type::Slider,
                   static_cast<int>(opacity_ * 100.0F),
                   0.0F,                                                                   100.0F,
                   1.0F                                                                                }
    };
}

void PencilTool::setOptionValue(const std::string& optionId,
                                const std::variant<int, float, bool, std::string>& value)
{
    if (optionId == "brush_size" && std::holds_alternative<int>(value)) {
        setBrushSize(std::get<int>(value));
    } else if (optionId == "opacity" && std::holds_alternative<int>(value)) {
        opacity_ = static_cast<float>(std::get<int>(value)) / 100.0F;
    }
}

std::variant<int, float, bool, std::string> PencilTool::getOptionValue(
    const std::string& optionId) const
{
    if (optionId == "brush_size") {
        return brushSize_;
    } else if (optionId == "opacity") {
        return static_cast<int>(opacity_ * 100.0F);
    }
    return 0;
}

}  // namespace gimp
