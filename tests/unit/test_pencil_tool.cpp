/**
 * @file test_pencil_tool.cpp
 * @brief Unit tests for PencilTool.
 * @author Laurent Jiang
 * @date 2026-01-28
 */

#include "core/tools/pencil_tool.h"

#include "core/command_bus.h"
#include "core/layer.h"
#include "io/project_file.h"

#include <catch2/catch_test_macros.hpp>

// ============================================================================
// Basic Property Tests
// ============================================================================

TEST_CASE("PencilTool has correct id", "[pencil_tool][unit]")
{
    gimp::PencilTool tool;
    REQUIRE(tool.id() == "pencil");
}

TEST_CASE("PencilTool has correct name", "[pencil_tool][unit]")
{
    gimp::PencilTool tool;
    REQUIRE(tool.name() == "Pencil");
}

TEST_CASE("PencilTool default brush size is 3", "[pencil_tool][unit]")
{
    gimp::PencilTool tool;
    REQUIRE(tool.brushSize() == 3);
}

TEST_CASE("PencilTool setBrushSize updates brush size", "[pencil_tool][unit]")
{
    gimp::PencilTool tool;

    tool.setBrushSize(25);
    REQUIRE(tool.brushSize() == 25);

    tool.setBrushSize(1);
    REQUIRE(tool.brushSize() == 1);

    tool.setBrushSize(100);
    REQUIRE(tool.brushSize() == 100);
}

TEST_CASE("PencilTool color uses global foreground color", "[pencil_tool][unit]")
{
    gimp::PencilTool tool;

    tool.setColor(0xFF0000FF);  // Red
    REQUIRE(tool.color() == 0xFF0000FF);

    tool.setColor(0x00FF00FF);  // Green
    REQUIRE(tool.color() == 0x00FF00FF);
}

// ============================================================================
// State Machine Tests
// ============================================================================

TEST_CASE("PencilTool starts in Idle state", "[pencil_tool][unit]")
{
    gimp::PencilTool tool;
    REQUIRE(tool.state() == gimp::ToolState::Idle);
}

TEST_CASE("PencilTool transitions to Active on mouse press", "[pencil_tool][unit]")
{
    gimp::PencilTool tool;
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

TEST_CASE("PencilTool stays Active during mouse move", "[pencil_tool][unit]")
{
    gimp::PencilTool tool;
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

TEST_CASE("PencilTool returns to Idle after mouse release", "[pencil_tool][unit]")
{
    gimp::PencilTool tool;
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

TEST_CASE("PencilTool reset cancels stroke and returns to Idle", "[pencil_tool][unit]")
{
    gimp::PencilTool tool;
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

TEST_CASE("PencilTool modifies layer pixels", "[pencil_tool][unit]")
{
    gimp::PencilTool tool;
    tool.setBrushSize(10);
    tool.setColor(0xFF0000FF);  // Red, full opacity

    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    tool.setDocument(doc);

    // Layer starts transparent/black
    auto layer = doc->layers()[0];
    auto& data = layer->data();
    int centerIdx = (50 * 100 + 50) * 4;
    uint8_t originalR = data[centerIdx];

    // Perform pencil stroke at center
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

    // Check that pixel was drawn (should have red color now)
    uint8_t newR = data[centerIdx];
    REQUIRE(newR != originalR);
}

TEST_CASE("PencilTool handles empty document gracefully", "[pencil_tool][unit]")
{
    gimp::PencilTool tool;

    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(50, 50);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    REQUIRE_NOTHROW(tool.onMousePress(pressEvent));
}

TEST_CASE("PencilTool handles document with no layers", "[pencil_tool][unit]")
{
    gimp::PencilTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    tool.setDocument(doc);

    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(50, 50);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    REQUIRE_NOTHROW(tool.onMousePress(pressEvent));
}

