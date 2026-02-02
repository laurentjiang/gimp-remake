/**
 * @file brush_tool.cpp
 * @brief Implementation of BrushTool.
 * @author Laurent Jiang
 * @date 2026-02-02
 */

#include "core/tools/brush_tool.h"

#include "core/command_bus.h"
#include "core/commands/draw_command.h"
#include "core/document.h"
#include "core/layer.h"
#include "core/tool_factory.h"

#include <algorithm>
#include <cmath>

namespace gimp {

namespace {

/**
 * @brief Interpolates points along a line between two stroke points.
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

BrushTool::BrushTool() : brush_(std::make_unique<SoftBrush>())
{
    brush_->setHardness(hardness_);
}

void BrushTool::setHardness(float hardness)
{
    hardness_ = std::clamp(hardness, 0.0F, 1.0F);
    brush_->setHardness(hardness_);
}

void BrushTool::setOpacity(float opacity)
{
    opacity_ = std::clamp(opacity, 0.0F, 1.0F);
}

void BrushTool::setVelocityDynamics(bool enabled)
{
    dynamics_.config().useVelocity = enabled;
    // When using velocity dynamics, don't use raw tablet pressure
    dynamics_.config().usePressure = !enabled;
}

void BrushTool::renderSegment(int fromX,
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

    std::uint32_t color = ToolFactory::instance().foregroundColor();
    // Apply opacity to the alpha channel
    std::uint8_t colorAlpha = static_cast<std::uint8_t>(color & 0xFF);
    std::uint8_t adjustedAlpha =
        static_cast<std::uint8_t>(static_cast<float>(colorAlpha) * opacity_);
    color = (color & 0xFFFFFF00) | adjustedAlpha;

    auto interpolated =
        interpolatePoints(fromX, fromY, fromPressure, toX, toY, toPressure, brushSize_);

    for (const auto& [x, y, pressure] : interpolated) {
        brush_->renderDab(pixelData, layerWidth, layerHeight, x, y, brushSize_, color, pressure);
    }
}

void BrushTool::beginStroke(const ToolInputEvent& event)
{
    strokePoints_.clear();
    beforeState_.clear();
    dynamics_.beginStroke();

    if (!document_ || document_->layers().count() == 0) {
        return;
    }

    auto layer = document_->layers()[0];
    beforeState_ = layer->data();

    // Compute initial pressure from dynamics
    DynamicsInput dynInput = dynamics_.update(event.canvasPos.x(), event.canvasPos.y(), event.pressure);
    float effectivePressure = dynamics_.computePressure(dynInput);

    strokePoints_.push_back({event.canvasPos.x(), event.canvasPos.y(), effectivePressure});

    auto* pixelData = layer->data().data();
    int layerWidth = layer->width();
    int layerHeight = layer->height();

    std::uint32_t color = ToolFactory::instance().foregroundColor();
    std::uint8_t colorAlpha = static_cast<std::uint8_t>(color & 0xFF);
    std::uint8_t adjustedAlpha =
        static_cast<std::uint8_t>(static_cast<float>(colorAlpha) * opacity_);
    color = (color & 0xFFFFFF00) | adjustedAlpha;

    brush_->renderDab(pixelData,
                      layerWidth,
                      layerHeight,
                      event.canvasPos.x(),
                      event.canvasPos.y(),
                      brushSize_,
                      color,
                      effectivePressure);
}

void BrushTool::continueStroke(const ToolInputEvent& event)
{
    if (strokePoints_.empty()) {
        return;
    }

    const auto& lastPoint = strokePoints_.back();
    int newX = event.canvasPos.x();
    int newY = event.canvasPos.y();

    if (newX != lastPoint.x || newY != lastPoint.y) {
        // Compute pressure from dynamics
        DynamicsInput dynInput = dynamics_.update(newX, newY, event.pressure);
        float effectivePressure = dynamics_.computePressure(dynInput);

        renderSegment(lastPoint.x, lastPoint.y, lastPoint.pressure, newX, newY, effectivePressure);
        strokePoints_.push_back({newX, newY, effectivePressure});
    }
}

std::shared_ptr<DrawCommand> BrushTool::buildDrawCommand(int minX, int maxX, int minY, int maxY)
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

void BrushTool::endStroke(const ToolInputEvent& event)
{
    if (strokePoints_.empty() || beforeState_.empty()) {
        strokePoints_.clear();
        beforeState_.clear();
        return;
    }

    const auto& lastPoint = strokePoints_.back();
    int newX = event.canvasPos.x();
    int newY = event.canvasPos.y();
    if (newX != lastPoint.x || newY != lastPoint.y) {
        DynamicsInput dynInput = dynamics_.update(newX, newY, event.pressure);
        float effectivePressure = dynamics_.computePressure(dynInput);
        renderSegment(lastPoint.x, lastPoint.y, lastPoint.pressure, newX, newY, effectivePressure);
        strokePoints_.push_back({newX, newY, effectivePressure});
    }

    if (!document_ || !commandBus_) {
        strokePoints_.clear();
        beforeState_.clear();
        return;
    }

    auto drawCmd = buildDrawCommand(INT_MAX, INT_MIN, INT_MAX, INT_MIN);
    if (!drawCmd) {
        beforeState_.clear();
        return;
    }

    auto layer = document_->layers()[0];
    std::vector<uint8_t> afterState = layer->data();

    layer->data() = beforeState_;
    drawCmd->captureBeforeState();

    layer->data() = afterState;
    drawCmd->captureAfterState();

    commandBus_->dispatch(drawCmd);

    ToolFactory::instance().markForegroundColorUsed();

    strokePoints_.clear();
    beforeState_.clear();
}

void BrushTool::cancelStroke()
{
    strokePoints_.clear();
}

}  // namespace gimp
