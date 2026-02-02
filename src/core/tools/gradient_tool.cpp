/**
 * @file gradient_tool.cpp
 * @brief Implementation of GradientTool.
 * @author Laurent Jiang
 * @date 2026-02-02
 */

#include "core/tools/gradient_tool.h"

#include "core/command_bus.h"
#include "core/commands/draw_command.h"
#include "core/document.h"
#include "core/layer.h"
#include "core/tool_factory.h"

#include <cmath>
#include <algorithm>

namespace gimp {

std::string GradientTool::id() const
{
    return "gradient";
}

std::string GradientTool::name() const
{
    return "Gradient";
}

void GradientTool::setGradientShape(int startX, int startY, int endX, int endY)
{
    startX_ = startX;
    startY_ = startY;
    endX_ = endX;
    endY_ = endY;
}

QPoint GradientTool::gradientStart() const
{
    return QPoint(startX_, startY_);
}

QPoint GradientTool::gradientEnd() const
{
    return QPoint(endX_, endY_);
}

void GradientTool::beginStroke(const ToolInputEvent& event)
{
    auto doc = document();
    if (!doc || doc->layers().count() == 0) {
        return;
    }

    auto layer = doc->layers()[0];
    if (!layer) {
        return;
    }

    // Store initial gradient shape
    startX_ = event.canvasPos.x();
    startY_ = event.canvasPos.y();
    endX_ = event.canvasPos.x();
    endY_ = event.canvasPos.y();

    // Capture before state
    command_ = std::make_shared<DrawCommand>(layer, 0, 0, layer->width(), layer->height());
    command_->captureBeforeState();
}

void GradientTool::continueStroke(const ToolInputEvent& event)
{
    // Update gradient end point
    endX_ = event.canvasPos.x();
    endY_ = event.canvasPos.y();
}

void GradientTool::endStroke(const ToolInputEvent& event)
{
    auto doc = document();
    if (!doc || doc->layers().count() == 0 || !command_) {
        beforeState_.clear();
        command_ = nullptr;
        return;
    }

    auto layer = doc->layers()[0];
    if (!layer || !commandBus_) {
        beforeState_.clear();
        command_ = nullptr;
        return;
    }

    // Update final gradient end point
    endX_ = event.canvasPos.x();
    endY_ = event.canvasPos.y();

    // Get colors from ToolFactory
    auto& factory = ToolFactory::instance();
    std::uint32_t foregroundColor = factory.foregroundColor();
    std::uint32_t backgroundColor = factory.backgroundColor();

    // Choose end color based on fill mode
    std::uint32_t endColor = (fill_ == GradientFill::ForegroundToBackground)
                                 ? backgroundColor
                                 : 0x00000000;  // Transparent

    // Apply gradient
    if (mode_ == GradientMode::Linear) {
        applyLinearGradient(layer, foregroundColor, endColor);
    } else {
        applyRadialGradient(layer, foregroundColor, endColor);
    }

    // Capture after state and dispatch command
    command_->captureAfterState();
    commandBus_->dispatch(command_);

    beforeState_.clear();
    command_ = nullptr;
}

void GradientTool::cancelStroke()
{
    beforeState_.clear();
    command_ = nullptr;
}

void GradientTool::applyLinearGradient(std::shared_ptr<Layer> layer,
                                       std::uint32_t startColor,
                                       std::uint32_t endColor)
{
    if (!layer) {
        return;
    }

    auto& data = layer->data();
    int width = layer->width();
    int height = layer->height();

    if (data.empty() || width <= 0 || height <= 0) {
        return;
    }

    // Calculate gradient vector
    float dx = static_cast<float>(endX_ - startX_);
    float dy = static_cast<float>(endY_ - startY_);
    float distSq = dx * dx + dy * dy;

    // Avoid division by zero
    if (distSq < 0.001F) {
        // Degenerate case: start and end are same, just fill with start color
        std::uint32_t r = (startColor >> 24) & 0xFF;
        std::uint32_t g = (startColor >> 16) & 0xFF;
        std::uint32_t b = (startColor >> 8) & 0xFF;
        std::uint32_t a = startColor & 0xFF;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 4;
                data[idx] = r;
                data[idx + 1] = g;
                data[idx + 2] = b;
                data[idx + 3] = a;
            }
        }
        return;
    }

    // Fill each pixel with interpolated color
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Vector from start point to current pixel
            float px = static_cast<float>(x - startX_);
            float py = static_cast<float>(y - startY_);

            // Project onto gradient vector
            float t = (px * dx + py * dy) / distSq;
            t = std::clamp(t, 0.0F, 1.0F);

            // Interpolate color
            std::uint32_t color = lerpColor(startColor, endColor, t);

            int idx = (y * width + x) * 4;
            data[idx] = (color >> 24) & 0xFF;
            data[idx + 1] = (color >> 16) & 0xFF;
            data[idx + 2] = (color >> 8) & 0xFF;
            data[idx + 3] = color & 0xFF;
        }
    }
}

void GradientTool::applyRadialGradient(std::shared_ptr<Layer> layer,
                                       std::uint32_t startColor,
                                       std::uint32_t endColor)
{
    if (!layer) {
        return;
    }

    auto& data = layer->data();
    int width = layer->width();
    int height = layer->height();

    if (data.empty() || width <= 0 || height <= 0) {
        return;
    }

    // Center and radius
    float cx = static_cast<float>(startX_);
    float cy = static_cast<float>(startY_);

    float dx = static_cast<float>(endX_ - startX_);
    float dy = static_cast<float>(endY_ - startY_);
    float radius = std::sqrt(dx * dx + dy * dy);

    // Avoid division by zero
    if (radius < 0.001F) {
        // Degenerate case: fill with start color
        std::uint32_t r = (startColor >> 24) & 0xFF;
        std::uint32_t g = (startColor >> 16) & 0xFF;
        std::uint32_t b = (startColor >> 8) & 0xFF;
        std::uint32_t a = startColor & 0xFF;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 4;
                data[idx] = r;
                data[idx + 1] = g;
                data[idx + 2] = b;
                data[idx + 3] = a;
            }
        }
        return;
    }

    // Fill each pixel with interpolated color based on distance from center
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float px = static_cast<float>(x) - cx;
            float py = static_cast<float>(y) - cy;
            float dist = std::sqrt(px * px + py * py);

            float t = dist / radius;
            t = std::clamp(t, 0.0F, 1.0F);

            // Interpolate color
            std::uint32_t color = lerpColor(startColor, endColor, t);

            int idx = (y * width + x) * 4;
            data[idx] = (color >> 24) & 0xFF;
            data[idx + 1] = (color >> 16) & 0xFF;
            data[idx + 2] = (color >> 8) & 0xFF;
            data[idx + 3] = color & 0xFF;
        }
    }
}

std::uint32_t GradientTool::lerpColor(std::uint32_t color1, std::uint32_t color2, float t)
{
    std::uint8_t r1 = (color1 >> 24) & 0xFF;
    std::uint8_t g1 = (color1 >> 16) & 0xFF;
    std::uint8_t b1 = (color1 >> 8) & 0xFF;
    std::uint8_t a1 = color1 & 0xFF;

    std::uint8_t r2 = (color2 >> 24) & 0xFF;
    std::uint8_t g2 = (color2 >> 16) & 0xFF;
    std::uint8_t b2 = (color2 >> 8) & 0xFF;
    std::uint8_t a2 = color2 & 0xFF;

    std::uint8_t r = static_cast<std::uint8_t>(r1 * (1.0F - t) + r2 * t);
    std::uint8_t g = static_cast<std::uint8_t>(g1 * (1.0F - t) + g2 * t);
    std::uint8_t b = static_cast<std::uint8_t>(b1 * (1.0F - t) + b2 * t);
    std::uint8_t a = static_cast<std::uint8_t>(a1 * (1.0F - t) + a2 * t);

    return (static_cast<std::uint32_t>(r) << 24) | (static_cast<std::uint32_t>(g) << 16) |
           (static_cast<std::uint32_t>(b) << 8) | static_cast<std::uint32_t>(a);
}

}  // namespace gimp
