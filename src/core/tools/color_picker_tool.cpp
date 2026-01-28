/**
 * @file color_picker_tool.cpp
 * @brief Implementation of ColorPickerTool.
 * @author Laurent Jiang
 * @date 2026-01-28
 */

#include "core/tools/color_picker_tool.h"

#include "core/document.h"
#include "core/event_bus.h"
#include "core/events.h"
#include "core/layer.h"

namespace gimp {

std::uint32_t ColorPickerTool::sampleColorAt(int x, int y) const
{
    if (!document_ || document_->layers().count() == 0) {
        return 0x000000FF;  // Default to opaque black
    }

    auto layer = document_->layers()[0];
    const int width = layer->width();
    const int height = layer->height();

    // Bounds check
    if (x < 0 || x >= width || y < 0 || y >= height) {
        return 0x000000FF;  // Default to opaque black
    }

    const auto& data = layer->data();
    const std::size_t offset =
        (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4;

    // RGBA format: extract each channel
    const std::uint8_t r = data[offset + 0];
    const std::uint8_t g = data[offset + 1];
    const std::uint8_t b = data[offset + 2];
    const std::uint8_t a = data[offset + 3];

    // Pack into 0xRRGGBBAA format
    return (static_cast<std::uint32_t>(r) << 24) | (static_cast<std::uint32_t>(g) << 16) |
           (static_cast<std::uint32_t>(b) << 8) | static_cast<std::uint32_t>(a);
}

void ColorPickerTool::publishColorChanged(std::uint32_t color) const
{
    ColorChangedEvent event;
    event.color = color;
    event.source = "color_picker";
    EventBus::instance().publish(event);
}

void ColorPickerTool::beginStroke(const ToolInputEvent& event)
{
    pickedColor_ = sampleColorAt(event.canvasPos.x(), event.canvasPos.y());
    publishColorChanged(pickedColor_);
}

void ColorPickerTool::continueStroke(const ToolInputEvent& event)
{
    // Live preview: update color as user drags
    pickedColor_ = sampleColorAt(event.canvasPos.x(), event.canvasPos.y());
    publishColorChanged(pickedColor_);
}

void ColorPickerTool::endStroke(const ToolInputEvent& event)
{
    // Final pick on mouse release
    pickedColor_ = sampleColorAt(event.canvasPos.x(), event.canvasPos.y());
    publishColorChanged(pickedColor_);
}

}  // namespace gimp
