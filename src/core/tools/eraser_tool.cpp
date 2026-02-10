/**
 * @file eraser_tool.cpp
 * @brief Implementation of EraserTool.
 * @author Laurent Jiang
 * @date 2026-01-28
 */

#include "core/tools/eraser_tool.h"

#include "core/command_bus.h"
#include "core/commands/draw_command.h"
#include "core/document.h"
#include "core/layer.h"
#include "core/tool_options.h"

#include <algorithm>
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

void EraserTool::eraseAt(int x, int y, float pressure)
{
    if (!activeLayer_) {
        return;
    }

    auto* pixelData = activeLayer_->data().data();
    int layerWidth = activeLayer_->width();
    int layerHeight = activeLayer_->height();

    int radius = brushSize_ / 2;
    int radiusSq = radius * radius;

    int minX = std::max(0, x - radius);
    int maxX = std::min(layerWidth - 1, x + radius);
    int minY = std::max(0, y - radius);
    int maxY = std::min(layerHeight - 1, y + radius);

    for (int py = minY; py <= maxY; ++py) {
        for (int px = minX; px <= maxX; ++px) {
            int dx = px - x;
            int dy = py - y;
            int distSq = dx * dx + dy * dy;
            if (distSq <= radiusSq) {
                // Calculate distance-based falloff for softness
                float dist = std::sqrt(static_cast<float>(distSq));
                float normalizedDist = (radius > 0) ? dist / static_cast<float>(radius) : 0.0F;

                // Apply hardness to falloff
                // hardness=1.0: hard edge (full strength until the edge)
                // hardness=0.0: soft edge (linear falloff from center)
                float edgeFalloff = 1.0F;
                if (hardness_ < 1.0F && normalizedDist > hardness_) {
                    edgeFalloff = 1.0F - (normalizedDist - hardness_) / (1.0F - hardness_ + 0.001F);
                    edgeFalloff = std::max(0.0F, edgeFalloff);
                }

                // Final erase strength combines pressure, opacity, and edge falloff
                float eraseStrength = pressure * opacity_ * edgeFalloff;

                std::uint8_t* pixel = pixelData + (py * layerWidth + px) * 4;
                // Erase by blending towards white background
                pixel[0] = static_cast<std::uint8_t>(
                    static_cast<float>(pixel[0]) * (1.0F - eraseStrength) + 255.0F * eraseStrength);
                pixel[1] = static_cast<std::uint8_t>(
                    static_cast<float>(pixel[1]) * (1.0F - eraseStrength) + 255.0F * eraseStrength);
                pixel[2] = static_cast<std::uint8_t>(
                    static_cast<float>(pixel[2]) * (1.0F - eraseStrength) + 255.0F * eraseStrength);
                pixel[3] = 255;  // Keep fully opaque
            }
        }
    }
}

void EraserTool::renderSegment(int fromX,
                               int fromY,
                               float fromPressure,
                               int toX,
                               int toY,
                               float toPressure)
{
    auto interpolated =
        interpolatePoints(fromX, fromY, fromPressure, toX, toY, toPressure, brushSize_);

    for (const auto& [x, y, pressure] : interpolated) {
        eraseAt(x, y, pressure);
    }
}

void EraserTool::beginStroke(const ToolInputEvent& event)
{
    strokePoints_.clear();
    beforeState_.clear();
    activeLayer_ = nullptr;

    if (!document_ || document_->layers().count() == 0) {
        return;
    }

    // Capture the layer state before we start erasing
    activeLayer_ = document_->activeLayer();
    if (!activeLayer_) {
        return;
    }
    beforeState_ = activeLayer_->data();

    // Add first point and erase it
    strokePoints_.push_back({event.canvasPos.x(), event.canvasPos.y(), event.pressure});
    eraseAt(event.canvasPos.x(), event.canvasPos.y(), event.pressure);
}

void EraserTool::continueStroke(const ToolInputEvent& event)
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

std::shared_ptr<DrawCommand> EraserTool::buildDrawCommand(int minX, int maxX, int minY, int maxY)
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

    return std::make_shared<DrawCommand>(activeLayer_, minX, minY, width, height);
}

void EraserTool::endStroke(const ToolInputEvent& event)
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

    if (!document_ || !commandBus_ || !activeLayer_) {
        strokePoints_.clear();
        beforeState_.clear();
        activeLayer_ = nullptr;
        return;
    }

    // Create command for the affected region
    auto drawCmd = buildDrawCommand(INT_MAX, INT_MIN, INT_MAX, INT_MIN);
    if (!drawCmd) {
        beforeState_.clear();
        activeLayer_ = nullptr;
        return;
    }

    // The layer now has the "after" state (with the erased pixels)
    // We need to swap in the "before" state, capture it, then swap back
    std::vector<uint8_t> afterState = activeLayer_->data();

    // Temporarily restore before state
    activeLayer_->data() = beforeState_;
    drawCmd->captureBeforeState();

    // Restore after state (the erased stroke)
    activeLayer_->data() = afterState;
    drawCmd->captureAfterState();

    commandBus_->dispatch(drawCmd);

    strokePoints_.clear();
    beforeState_.clear();
    activeLayer_ = nullptr;
}

void EraserTool::cancelStroke()
{
    strokePoints_.clear();
}

std::vector<ToolOption> EraserTool::getOptions() const
{
    return {
        ToolOption{
                   "brush_size", "Size", ToolOption::Type::Slider, brushSize_, 1.0F, 1000.0F, 1.0F, {}, 0},
        ToolOption{"opacity",
                   "Opacity",            ToolOption::Type::Slider,
                   static_cast<int>(opacity_ * 100.0F),
                   0.0F,                                                             100.0F,
                   1.0F,                                                                            {},
                   0                                                                                     },
        ToolOption{"hardness",
                   "Hardness",           ToolOption::Type::Slider,
                   static_cast<int>(hardness_ * 100.0F),
                   0.0F,                                                             100.0F,
                   1.0F,                                                                            {},
                   0                                                                                     }
    };
}

void EraserTool::setOptionValue(const std::string& optionId,
                                const std::variant<int, float, bool, std::string>& value)
{
    if (optionId == "brush_size" && std::holds_alternative<int>(value)) {
        setBrushSize(std::get<int>(value));
    } else if (optionId == "opacity" && std::holds_alternative<int>(value)) {
        setOpacity(static_cast<float>(std::get<int>(value)) / 100.0F);
    } else if (optionId == "hardness" && std::holds_alternative<int>(value)) {
        setHardness(static_cast<float>(std::get<int>(value)) / 100.0F);
    }
}

std::variant<int, float, bool, std::string> EraserTool::getOptionValue(
    const std::string& optionId) const
{
    if (optionId == "brush_size") {
        return brushSize_;
    }
    if (optionId == "opacity") {
        return static_cast<int>(opacity_ * 100.0F);
    }
    if (optionId == "hardness") {
        return static_cast<int>(hardness_ * 100.0F);
    }
    return 0;
}

}  // namespace gimp
