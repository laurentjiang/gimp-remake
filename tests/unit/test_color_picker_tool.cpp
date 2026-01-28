/**
 * @file test_color_picker_tool.cpp
 * @brief Unit tests for ColorPickerTool.
 * @author Laurent Jiang
 * @date 2026-01-28
 */

#include "core/event_bus.h"
#include "core/events.h"
#include "core/layer.h"
#include "core/layer_stack.h"
#include "core/tool_factory.h"
#include "core/tools/color_picker_tool.h"
#include "io/project_file.h"

#include <catch2/catch_test_macros.hpp>

namespace {

gimp::ToolInputEvent makeEvent(int x, int y, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
{
    gimp::ToolInputEvent event;
    event.canvasPos = QPoint(x, y);
    event.screenPos = QPoint(x, y);
    event.buttons = Qt::LeftButton;
    event.modifiers = modifiers;
    event.pressure = 1.0F;
    return event;
}

std::shared_ptr<gimp::ProjectFile> createTestDocument(int width, int height)
{
    auto doc = std::make_shared<gimp::ProjectFile>(width, height);
    doc->addLayer();
    return doc;
}

void fillLayerWithColor(const std::shared_ptr<gimp::Layer>& layer, std::uint32_t rgba)
{
    auto& data = layer->data();
    int pixelCount = layer->width() * layer->height();
    for (int i = 0; i < pixelCount; ++i) {
        data[static_cast<std::size_t>(i * 4)] = static_cast<std::uint8_t>((rgba >> 24) & 0xFF);
        data[static_cast<std::size_t>(i * 4 + 1)] = static_cast<std::uint8_t>((rgba >> 16) & 0xFF);
        data[static_cast<std::size_t>(i * 4 + 2)] = static_cast<std::uint8_t>((rgba >> 8) & 0xFF);
        data[static_cast<std::size_t>(i * 4 + 3)] = static_cast<std::uint8_t>(rgba & 0xFF);
    }
}

}  // namespace

TEST_CASE("ColorPickerTool has correct id and name", "[color_picker][unit]")
{
    gimp::ColorPickerTool picker;

    REQUIRE(picker.id() == "color_picker");
    REQUIRE(picker.name() == "Color Picker");
}

TEST_CASE("ColorPickerTool samples pixel color from layer", "[color_picker][unit]")
{
    gimp::EventBus::instance().clear();

    auto doc = createTestDocument(10, 10);
    auto layer = doc->layers()[0];

    const std::uint32_t testColor = 0xFF0000FF;  // Red, fully opaque
    fillLayerWithColor(layer, testColor);

    gimp::ColorPickerTool picker;
    picker.setDocument(doc);

    std::uint32_t pickedColor = 0;
    auto subId = gimp::EventBus::instance().subscribe<gimp::ColorChangedEvent>(
        [&pickedColor](const gimp::ColorChangedEvent& event) { pickedColor = event.color; });

    auto event = makeEvent(5, 5);
    picker.onMousePress(event);
    picker.onMouseRelease(event);

    REQUIRE(pickedColor == testColor);
    gimp::EventBus::instance().unsubscribe(subId);
}

TEST_CASE("ColorPickerTool updates ToolFactory foreground color", "[color_picker][unit]")
{
    gimp::EventBus::instance().clear();

    auto doc = createTestDocument(10, 10);
    auto layer = doc->layers()[0];

    const std::uint32_t testColor = 0x00FF00FF;  // Green, fully opaque
    fillLayerWithColor(layer, testColor);

    gimp::ColorPickerTool picker;
    picker.setDocument(doc);

    gimp::ToolFactory::instance().setForegroundColor(0x000000FF);  // Reset to black

    auto subId = gimp::EventBus::instance().subscribe<gimp::ColorChangedEvent>(
        [](const gimp::ColorChangedEvent& event) {
            gimp::ToolFactory::instance().setForegroundColor(event.color);
        });

    auto event = makeEvent(5, 5);
    picker.onMousePress(event);
    picker.onMouseRelease(event);

    REQUIRE(gimp::ToolFactory::instance().foregroundColor() == testColor);
    gimp::EventBus::instance().unsubscribe(subId);
}

TEST_CASE("ColorPickerTool handles out-of-bounds coordinates", "[color_picker][unit]")
{
    gimp::EventBus::instance().clear();

    auto doc = createTestDocument(10, 10);
    auto layer = doc->layers()[0];
    fillLayerWithColor(layer, 0xFF0000FF);

    gimp::ColorPickerTool picker;
    picker.setDocument(doc);

    bool eventFired = false;
    auto subId = gimp::EventBus::instance().subscribe<gimp::ColorChangedEvent>(
        [&eventFired](const gimp::ColorChangedEvent& /*event*/) { eventFired = true; });

    auto event = makeEvent(-1, -1);  // Out of bounds
    picker.onMousePress(event);
    picker.onMouseRelease(event);

    REQUIRE_FALSE(eventFired);
    gimp::EventBus::instance().unsubscribe(subId);
}

TEST_CASE("ColorPickerTool tracks previous tool for switch-back", "[color_picker][unit]")
{
    gimp::EventBus::instance().clear();

    gimp::ColorPickerTool picker;

    picker.setPreviousTool("pencil");
    picker.onActivate();

    bool switchRequested = false;
    std::string targetId;
    auto subId = gimp::EventBus::instance().subscribe<gimp::ToolSwitchRequestEvent>(
        [&](const gimp::ToolSwitchRequestEvent& event) {
            switchRequested = true;
            targetId = event.targetToolId;
        });

    auto doc = createTestDocument(10, 10);
    fillLayerWithColor(doc->layers()[0], 0xFF0000FF);
    picker.setDocument(doc);

    auto event = makeEvent(5, 5);
    picker.onMousePress(event);
    picker.onMouseRelease(event);

    REQUIRE(switchRequested);
    REQUIRE(targetId == "pencil");
    gimp::EventBus::instance().unsubscribe(subId);
}

TEST_CASE("ColorPickerTool samples correct color at specific pixel", "[color_picker][unit]")
{
    gimp::EventBus::instance().clear();

    auto doc = createTestDocument(10, 10);
    auto layer = doc->layers()[0];

    // Set a specific pixel to a unique color
    auto& data = layer->data();
    int targetX = 3;
    int targetY = 4;
    int index = (targetY * 10 + targetX) * 4;
    data[static_cast<std::size_t>(index)] = 0x12;      // R
    data[static_cast<std::size_t>(index + 1)] = 0x34;  // G
    data[static_cast<std::size_t>(index + 2)] = 0x56;  // B
    data[static_cast<std::size_t>(index + 3)] = 0x78;  // A

    gimp::ColorPickerTool picker;
    picker.setDocument(doc);

    std::uint32_t pickedColor = 0;
    auto subId = gimp::EventBus::instance().subscribe<gimp::ColorChangedEvent>(
        [&pickedColor](const gimp::ColorChangedEvent& event) { pickedColor = event.color; });

    auto event = makeEvent(targetX, targetY);
    picker.onMousePress(event);
    picker.onMouseRelease(event);

    REQUIRE(pickedColor == 0x12345678);
    gimp::EventBus::instance().unsubscribe(subId);
}
