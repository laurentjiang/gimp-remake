/**
 * @file fill_tool.cpp
 * @brief Implementation of FillTool.
 * @author Laurent Jiang
 * @date 2026-01-29
 */

#include "core/tools/fill_tool.h"

#include "core/command_bus.h"
#include "core/commands/draw_command.h"
#include "core/document.h"
#include "core/layer.h"
#include "core/tool_factory.h"

#include <algorithm>
#include <cmath>
#include <stack>

namespace gimp {

void FillTool::setTolerance(int tolerance)
{
    tolerance_ = std::clamp(tolerance, 0, 255);
}

std::uint32_t FillTool::getPixelColor(const std::vector<uint8_t>& data, int x, int y, int width)
{
    std::size_t index = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                         static_cast<std::size_t>(x)) *
                        4;
    std::uint32_t r = data[index];
    std::uint32_t g = data[index + 1];
    std::uint32_t b = data[index + 2];
    std::uint32_t a = data[index + 3];
    return (r << 24) | (g << 16) | (b << 8) | a;
}

void FillTool::setPixelColor(std::vector<uint8_t>& data,
                             int x,
                             int y,
                             int width,
                             std::uint32_t color)
{
    std::size_t index = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                         static_cast<std::size_t>(x)) *
                        4;
    data[index] = static_cast<uint8_t>((color >> 24) & 0xFF);
    data[index + 1] = static_cast<uint8_t>((color >> 16) & 0xFF);
    data[index + 2] = static_cast<uint8_t>((color >> 8) & 0xFF);
    data[index + 3] = static_cast<uint8_t>(color & 0xFF);
}

bool FillTool::colorMatches(std::uint32_t pixelColor, std::uint32_t targetColor) const
{
    int pr = static_cast<int>((pixelColor >> 24) & 0xFF);
    int pg = static_cast<int>((pixelColor >> 16) & 0xFF);
    int pb = static_cast<int>((pixelColor >> 8) & 0xFF);
    int pa = static_cast<int>(pixelColor & 0xFF);

    int tr = static_cast<int>((targetColor >> 24) & 0xFF);
    int tg = static_cast<int>((targetColor >> 16) & 0xFF);
    int tb = static_cast<int>((targetColor >> 8) & 0xFF);
    int ta = static_cast<int>(targetColor & 0xFF);

    return std::abs(pr - tr) <= tolerance_ && std::abs(pg - tg) <= tolerance_ &&
           std::abs(pb - tb) <= tolerance_ && std::abs(pa - ta) <= tolerance_;
}

void FillTool::floodFill(int startX, int startY, std::uint32_t fillColor)
{
    if (!document_ || document_->layers().count() == 0) {
        return;
    }

    auto layer = document_->layers()[0];
    std::vector<uint8_t>& data = layer->data();
    int width = layer->width();
    int height = layer->height();

    if (startX < 0 || startX >= width || startY < 0 || startY >= height) {
        return;
    }

    std::uint32_t targetColor = getPixelColor(data, startX, startY, width);

    // If target is same as fill color, nothing to do
    if (targetColor == fillColor) {
        return;
    }

    // Scanline flood fill algorithm using a stack
    // Each entry is (x, y) - left-most pixel of a span to check
    std::stack<std::pair<int, int>> stack;
    stack.emplace(startX, startY);

    while (!stack.empty()) {
        auto [x, y] = stack.top();
        stack.pop();

        if (y < 0 || y >= height) {
            continue;
        }

        // Find left edge
        int left = x;
        while (left > 0 && colorMatches(getPixelColor(data, left - 1, y, width), targetColor)) {
            --left;
        }

        // Find right edge
        int right = x;
        while (right < width - 1 &&
               colorMatches(getPixelColor(data, right + 1, y, width), targetColor)) {
            ++right;
        }

        // Fill the span
        for (int px = left; px <= right; ++px) {
            if (colorMatches(getPixelColor(data, px, y, width), targetColor)) {
                setPixelColor(data, px, y, width, fillColor);
            }
        }

        // Check scanlines above and below
        bool aboveInside = false;
        bool belowInside = false;

        for (int px = left; px <= right; ++px) {
            // Check above
            if (y > 0) {
                std::uint32_t aboveColor = getPixelColor(data, px, y - 1, width);
                if (colorMatches(aboveColor, targetColor) && aboveColor != fillColor) {
                    if (!aboveInside) {
                        stack.emplace(px, y - 1);
                        aboveInside = true;
                    }
                } else {
                    aboveInside = false;
                }
            }

            // Check below
            if (y < height - 1) {
                std::uint32_t belowColor = getPixelColor(data, px, y + 1, width);
                if (colorMatches(belowColor, targetColor) && belowColor != fillColor) {
                    if (!belowInside) {
                        stack.emplace(px, y + 1);
                        belowInside = true;
                    }
                } else {
                    belowInside = false;
                }
            }
        }
    }
}

void FillTool::beginStroke(const ToolInputEvent& event)
{
    beforeState_.clear();
    fillPending_ = false;

    // Only fill on left mouse button
    if ((event.buttons & Qt::LeftButton) == 0) {
        return;
    }

    if (!document_ || document_->layers().count() == 0) {
        return;
    }

    // Capture the layer state before the fill
    auto layer = document_->layers()[0];
    beforeState_ = layer->data();

    // Perform the flood fill
    std::uint32_t fillColor = ToolFactory::instance().foregroundColor();
    floodFill(event.canvasPos.x(), event.canvasPos.y(), fillColor);

    fillPending_ = true;
}

void FillTool::continueStroke(const ToolInputEvent& /*event*/)
{
    // Fill tool doesn't do anything on drag
}

void FillTool::endStroke(const ToolInputEvent& /*event*/)
{
    if (!fillPending_ || beforeState_.empty()) {
        beforeState_.clear();
        fillPending_ = false;
        return;
    }

    if (!document_ || !commandBus_) {
        beforeState_.clear();
        fillPending_ = false;
        return;
    }

    auto layer = document_->layers()[0];
    int width = layer->width();
    int height = layer->height();

    // Create command for the entire layer (fill could affect any region)
    auto drawCmd = std::make_shared<DrawCommand>(layer, 0, 0, width, height);

    // The layer now has the "after" state (with the fill)
    // Swap in the "before" state, capture it, then swap back
    std::vector<uint8_t> afterState = layer->data();

    layer->data() = beforeState_;
    drawCmd->captureBeforeState();

    layer->data() = afterState;
    drawCmd->captureAfterState();

    commandBus_->dispatch(drawCmd);

    // Mark the foreground color as used for recent colors tracking
    ToolFactory::instance().markForegroundColorUsed();

    beforeState_.clear();
    fillPending_ = false;
}

void FillTool::cancelStroke()
{
    if (!beforeState_.empty() && document_ && document_->layers().count() > 0) {
        // Restore original state
        document_->layers()[0]->data() = beforeState_;
    }
    beforeState_.clear();
    fillPending_ = false;
}

std::vector<ToolOption> FillTool::getOptions() const
{
    return {
        ToolOption{
                   "tolerance", "Tolerance", ToolOption::Type::Slider, tolerance_, 0.0F, 255.0F, 1.0F}
    };
}

void FillTool::setOptionValue(const std::string& optionId,
                              const std::variant<int, float, bool, std::string>& value)
{
    if (optionId == "tolerance" && std::holds_alternative<int>(value)) {
        setTolerance(std::get<int>(value));
    }
}

std::variant<int, float, bool, std::string> FillTool::getOptionValue(
    const std::string& optionId) const
{
    if (optionId == "tolerance") {
        return tolerance_;
    }
    return 0;
}

}  // namespace gimp
