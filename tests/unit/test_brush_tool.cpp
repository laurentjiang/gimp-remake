/**
 * @file test_brush_tool.cpp
 * @brief Unit tests for BrushTool.
 * @author Laurent Jiang
 * @date 2026-02-02
 */

#include "core/command_bus.h"
#include "core/layer.h"
#include "core/tool_options.h"
#include "core/tools/brush_tool.h"
#include "io/project_file.h"

#include <catch2/catch_test_macros.hpp>

// ============================================================================
// Basic Property Tests
// ============================================================================

TEST_CASE("BrushTool has correct id", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    REQUIRE(tool.id() == "paintbrush");
}

TEST_CASE("BrushTool has correct name", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    REQUIRE(tool.name() == "Paintbrush");
}

TEST_CASE("BrushTool default brush size is 20", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    REQUIRE(tool.brushSize() == 20);
}

TEST_CASE("BrushTool setBrushSize updates brush size", "[brush_tool][unit]")
{
    gimp::BrushTool tool;

    tool.setBrushSize(50);
    REQUIRE(tool.brushSize() == 50);

    tool.setBrushSize(1);
    REQUIRE(tool.brushSize() == 1);

    tool.setBrushSize(200);
    REQUIRE(tool.brushSize() == 200);
}

TEST_CASE("BrushTool color uses global foreground color", "[brush_tool][unit]")
{
    gimp::BrushTool tool;

    tool.setColor(0xFF0000FF);  // Red
    REQUIRE(tool.color() == 0xFF0000FF);

    tool.setColor(0x00FF00FF);  // Green
    REQUIRE(tool.color() == 0x00FF00FF);
}

// ============================================================================
// Hardness Tests
// ============================================================================

TEST_CASE("BrushTool default hardness is 0.5", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    REQUIRE(tool.hardness() == 0.5F);
}

TEST_CASE("BrushTool setHardness updates hardness", "[brush_tool][unit]")
{
    gimp::BrushTool tool;

    tool.setHardness(0.0F);
    REQUIRE(tool.hardness() == 0.0F);

    tool.setHardness(1.0F);
    REQUIRE(tool.hardness() == 1.0F);

    tool.setHardness(0.75F);
    REQUIRE(tool.hardness() == 0.75F);
}

TEST_CASE("BrushTool setHardness clamps values to 0.0-1.0", "[brush_tool][unit]")
{
    gimp::BrushTool tool;

    tool.setHardness(-0.5F);
    REQUIRE(tool.hardness() == 0.0F);

    tool.setHardness(1.5F);
    REQUIRE(tool.hardness() == 1.0F);
}

// ============================================================================
// Opacity Tests
// ============================================================================

TEST_CASE("BrushTool default opacity is 1.0", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    REQUIRE(tool.opacity() == 1.0F);
}

TEST_CASE("BrushTool setOpacity updates opacity", "[brush_tool][unit]")
{
    gimp::BrushTool tool;

    tool.setOpacity(0.0F);
    REQUIRE(tool.opacity() == 0.0F);

    tool.setOpacity(0.5F);
    REQUIRE(tool.opacity() == 0.5F);

    tool.setOpacity(1.0F);
    REQUIRE(tool.opacity() == 1.0F);
}

TEST_CASE("BrushTool setOpacity clamps values to 0.0-1.0", "[brush_tool][unit]")
{
    gimp::BrushTool tool;

    tool.setOpacity(-0.5F);
    REQUIRE(tool.opacity() == 0.0F);

    tool.setOpacity(1.5F);
    REQUIRE(tool.opacity() == 1.0F);
}

// ============================================================================
// State Machine Tests
// ============================================================================

TEST_CASE("BrushTool starts in Idle state", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    REQUIRE(tool.state() == gimp::ToolState::Idle);
}

TEST_CASE("BrushTool transitions to Active on mouse press", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    tool.setDocument(doc);

    gimp::ToolInputEvent event;
    event.canvasPos = QPoint(50, 50);
    event.buttons = Qt::LeftButton;
    event.pressure = 1.0F;

    tool.onMousePress(event);

    REQUIRE(tool.state() == gimp::ToolState::Active);
}

TEST_CASE("BrushTool stays Active during mouse move", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    tool.setDocument(doc);

    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(50, 50);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    tool.onMousePress(pressEvent);

    gimp::ToolInputEvent moveEvent;
    moveEvent.canvasPos = QPoint(60, 60);
    moveEvent.buttons = Qt::LeftButton;
    moveEvent.pressure = 1.0F;

    tool.onMouseMove(moveEvent);

    REQUIRE(tool.state() == gimp::ToolState::Active);
}

TEST_CASE("BrushTool returns to Idle after mouse release", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    tool.setDocument(doc);

    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(50, 50);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    tool.onMousePress(pressEvent);

    gimp::ToolInputEvent releaseEvent;
    releaseEvent.canvasPos = QPoint(60, 60);
    releaseEvent.buttons = Qt::NoButton;
    releaseEvent.pressure = 1.0F;

    tool.onMouseRelease(releaseEvent);

    REQUIRE(tool.state() == gimp::ToolState::Idle);
}

TEST_CASE("BrushTool reset cancels stroke and returns to Idle", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    tool.setDocument(doc);

    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(50, 50);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    tool.onMousePress(pressEvent);
    REQUIRE(tool.state() == gimp::ToolState::Active);

    tool.reset();
    REQUIRE(tool.state() == gimp::ToolState::Idle);
}

// ============================================================================
// Drawing Tests
// ============================================================================

TEST_CASE("BrushTool modifies layer pixels", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    tool.setBrushSize(10);
    tool.setColor(0xFF0000FF);  // Red, full opacity

    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    tool.setDocument(doc);

    auto layer = doc->layers()[0];
    auto& data = layer->data();
    int centerIdx = (50 * 100 + 50) * 4;
    uint8_t originalR = data[centerIdx];

    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(50, 50);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    tool.onMousePress(pressEvent);

    gimp::ToolInputEvent releaseEvent;
    releaseEvent.canvasPos = QPoint(50, 50);
    releaseEvent.buttons = Qt::NoButton;
    releaseEvent.pressure = 1.0F;

    tool.onMouseRelease(releaseEvent);

    uint8_t newR = data[centerIdx];
    REQUIRE(newR != originalR);
}

TEST_CASE("BrushTool with soft hardness creates gradient edges", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    tool.setBrushSize(30);
    tool.setHardness(0.0F);  // Fully soft
    tool.setColor(0xFF0000FF);

    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    tool.setDocument(doc);

    auto layer = doc->layers()[0];
    auto& data = layer->data();

    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(50, 50);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    tool.onMousePress(pressEvent);

    gimp::ToolInputEvent releaseEvent;
    releaseEvent.canvasPos = QPoint(50, 50);
    releaseEvent.buttons = Qt::NoButton;
    releaseEvent.pressure = 1.0F;

    tool.onMouseRelease(releaseEvent);

    // Center pixel should be more opaque than edge pixels
    int centerIdx = (50 * 100 + 50) * 4;
    int edgeIdx = (50 * 100 + 35) * 4;  // 15 pixels from center

    uint8_t centerAlpha = data[centerIdx + 3];
    uint8_t edgeAlpha = data[edgeIdx + 3];

    // Soft brush should have gradient (center >= edge)
    REQUIRE(centerAlpha >= edgeAlpha);
}

TEST_CASE("BrushTool with low opacity creates semi-transparent strokes", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    tool.setBrushSize(10);
    tool.setOpacity(0.5F);
    tool.setHardness(1.0F);  // Hard edge for predictable results
    tool.setColor(0xFF0000FF);

    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    tool.setDocument(doc);

    auto layer = doc->layers()[0];
    auto& data = layer->data();

    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(50, 50);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    tool.onMousePress(pressEvent);

    gimp::ToolInputEvent releaseEvent;
    releaseEvent.canvasPos = QPoint(50, 50);
    releaseEvent.buttons = Qt::NoButton;
    releaseEvent.pressure = 1.0F;

    tool.onMouseRelease(releaseEvent);

    // Check that the alpha value is reduced due to 50% opacity
    int centerIdx = (50 * 100 + 50) * 4;
    uint8_t alpha = data[centerIdx + 3];

    // With 50% opacity, alpha should be less than 255 (but more than 0)
    REQUIRE(alpha < 255);
    REQUIRE(alpha > 0);
}

TEST_CASE("BrushTool handles empty document gracefully", "[brush_tool][unit]")
{
    gimp::BrushTool tool;

    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(50, 50);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    REQUIRE_NOTHROW(tool.onMousePress(pressEvent));
}

TEST_CASE("BrushTool handles document with no layers", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    tool.setDocument(doc);

    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(50, 50);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    REQUIRE_NOTHROW(tool.onMousePress(pressEvent));
}

// ============================================================================
// ToolOptions Interface Tests
// ============================================================================

TEST_CASE("BrushTool getOptions returns size, opacity, hardness, velocity_dynamics",
          "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    auto* toolOptions = dynamic_cast<gimp::ToolOptions*>(&tool);
    REQUIRE(toolOptions != nullptr);

    auto options = toolOptions->getOptions();

    REQUIRE(options.size() == 4);
    REQUIRE(options[0].id == "brush_size");
    REQUIRE(options[1].id == "opacity");
    REQUIRE(options[2].id == "hardness");
    REQUIRE(options[3].id == "velocity_dynamics");
}

TEST_CASE("BrushTool setOptionValue updates brush_size", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    auto* toolOptions = dynamic_cast<gimp::ToolOptions*>(&tool);
    REQUIRE(toolOptions != nullptr);

    toolOptions->setOptionValue("brush_size", 100);
    REQUIRE(tool.brushSize() == 100);

    auto value = toolOptions->getOptionValue("brush_size");
    REQUIRE(std::holds_alternative<int>(value));
    REQUIRE(std::get<int>(value) == 100);
}

TEST_CASE("BrushTool setOptionValue updates opacity", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    auto* toolOptions = dynamic_cast<gimp::ToolOptions*>(&tool);
    REQUIRE(toolOptions != nullptr);

    toolOptions->setOptionValue("opacity", 75);  // Stored as percentage
    REQUIRE(tool.opacity() == 0.75F);

    auto value = toolOptions->getOptionValue("opacity");
    REQUIRE(std::holds_alternative<int>(value));
    REQUIRE(std::get<int>(value) == 75);
}

TEST_CASE("BrushTool setOptionValue updates hardness", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    auto* toolOptions = dynamic_cast<gimp::ToolOptions*>(&tool);
    REQUIRE(toolOptions != nullptr);

    toolOptions->setOptionValue("hardness", 25);  // Stored as percentage
    REQUIRE(tool.hardness() == 0.25F);

    auto value = toolOptions->getOptionValue("hardness");
    REQUIRE(std::holds_alternative<int>(value));
    REQUIRE(std::get<int>(value) == 25);
}

TEST_CASE("BrushTool setOptionValue updates velocity_dynamics", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    auto* toolOptions = dynamic_cast<gimp::ToolOptions*>(&tool);
    REQUIRE(toolOptions != nullptr);

    REQUIRE(tool.velocityDynamics() == false);

    toolOptions->setOptionValue("velocity_dynamics", true);
    REQUIRE(tool.velocityDynamics() == true);

    auto value = toolOptions->getOptionValue("velocity_dynamics");
    REQUIRE(std::holds_alternative<bool>(value));
    REQUIRE(std::get<bool>(value) == true);
}

TEST_CASE("BrushTool getOptionValue returns 0 for unknown option", "[brush_tool][unit]")
{
    gimp::BrushTool tool;
    auto* toolOptions = dynamic_cast<gimp::ToolOptions*>(&tool);
    REQUIRE(toolOptions != nullptr);

    auto value = toolOptions->getOptionValue("unknown_option");
    REQUIRE(std::holds_alternative<int>(value));
    REQUIRE(std::get<int>(value) == 0);
}
