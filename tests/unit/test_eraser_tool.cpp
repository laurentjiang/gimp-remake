/**
 * @file test_eraser_tool.cpp
 * @brief Unit tests for EraserTool.
 * @author Laurent Jiang
 * @date 2026-01-28
 */

#include "core/command_bus.h"
#include "core/layer.h"
#include "core/tools/eraser_tool.h"
#include "io/project_file.h"

#include <catch2/catch_test_macros.hpp>

// ============================================================================
// Basic Property Tests
// ============================================================================

TEST_CASE("EraserTool has correct id", "[eraser_tool][unit]")
{
    gimp::EraserTool tool;
    REQUIRE(tool.id() == "eraser");
}

TEST_CASE("EraserTool has correct name", "[eraser_tool][unit]")
{
    gimp::EraserTool tool;
    REQUIRE(tool.name() == "Eraser");
}

TEST_CASE("EraserTool default brush size is 10", "[eraser_tool][unit]")
{
    gimp::EraserTool tool;
    REQUIRE(tool.brushSize() == 10);
}

TEST_CASE("EraserTool setBrushSize updates brush size", "[eraser_tool][unit]")
{
    gimp::EraserTool tool;

    tool.setBrushSize(25);
    REQUIRE(tool.brushSize() == 25);

    tool.setBrushSize(1);
    REQUIRE(tool.brushSize() == 1);

    tool.setBrushSize(100);
    REQUIRE(tool.brushSize() == 100);
}

// ============================================================================
// State Machine Tests
// ============================================================================

TEST_CASE("EraserTool starts in Idle state", "[eraser_tool][unit]")
{
    gimp::EraserTool tool;
    REQUIRE(tool.state() == gimp::ToolState::Idle);
}

TEST_CASE("EraserTool transitions to Active on mouse press", "[eraser_tool][unit]")
{
    gimp::EraserTool tool;
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

TEST_CASE("EraserTool stays Active during mouse move", "[eraser_tool][unit]")
{
    gimp::EraserTool tool;
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

TEST_CASE("EraserTool returns to Idle after mouse release", "[eraser_tool][unit]")
{
    gimp::EraserTool tool;
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

TEST_CASE("EraserTool reset cancels stroke and returns to Idle", "[eraser_tool][unit]")
{
    gimp::EraserTool tool;
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
// Erasing Tests
// ============================================================================

TEST_CASE("EraserTool modifies layer pixels", "[eraser_tool][unit]")
{
    gimp::EraserTool tool;
    tool.setBrushSize(10);

    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    tool.setDocument(doc);

    // Fill layer with red color
    auto layer = doc->layers()[0];
    auto& data = layer->data();
    for (size_t i = 0; i < data.size(); i += 4) {
        data[i] = 255;      // R
        data[i + 1] = 0;    // G
        data[i + 2] = 0;    // B
        data[i + 3] = 255;  // A
    }

    // Store original pixel value at center
    int centerIdx = (50 * 100 + 50) * 4;
    uint8_t originalR = data[centerIdx];
    REQUIRE(originalR == 255);

    // Perform eraser stroke at center
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

    // Check that pixel was erased (blended towards white)
    uint8_t newR = data[centerIdx];
    REQUIRE(newR == 255);  // White after full pressure erase
}

TEST_CASE("EraserTool handles empty document gracefully", "[eraser_tool][unit]")
{
    gimp::EraserTool tool;

    // No document set
    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(50, 50);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    // Should not crash
    REQUIRE_NOTHROW(tool.onMousePress(pressEvent));
}

TEST_CASE("EraserTool handles document with no layers", "[eraser_tool][unit]")
{
    gimp::EraserTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    tool.setDocument(doc);

    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(50, 50);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    // Should not crash
    REQUIRE_NOTHROW(tool.onMousePress(pressEvent));
}
