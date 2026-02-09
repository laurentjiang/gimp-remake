/**
 * @file test_free_select_tool.cpp
 * @brief Unit tests for FreeSelectTool.
 * @author Laurent Jiang
 * @date 2026-02-09
 */

#include "core/selection_manager.h"
#include "core/tools/free_select_tool.h"
#include "io/project_file.h"

#include <catch2/catch_test_macros.hpp>

// ============================================================================
// Basic Property Tests
// ============================================================================

TEST_CASE("FreeSelectTool has correct id", "[free_select_tool][unit]")
{
    gimp::FreeSelectTool tool;
    REQUIRE(tool.id() == "select_free");
}

TEST_CASE("FreeSelectTool has correct name", "[free_select_tool][unit]")
{
    gimp::FreeSelectTool tool;
    REQUIRE(tool.name() == "Free Select");
}

// ============================================================================
// State Machine Tests
// ============================================================================

TEST_CASE("FreeSelectTool starts in Idle state", "[free_select_tool][unit]")
{
    gimp::FreeSelectTool tool;
    REQUIRE(tool.state() == gimp::ToolState::Idle);
}

TEST_CASE("FreeSelectTool transitions to Active on mouse press", "[free_select_tool][unit]")
{
    gimp::FreeSelectTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    tool.setDocument(doc);

    gimp::ToolInputEvent event;
    event.canvasPos = QPoint(10, 10);
    event.buttons = Qt::LeftButton;
    event.pressure = 1.0F;

    tool.onMousePress(event);

    REQUIRE(tool.state() == gimp::ToolState::Active);
}

TEST_CASE("FreeSelectTool stays Active during mouse move", "[free_select_tool][unit]")
{
    gimp::FreeSelectTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    tool.setDocument(doc);

    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(10, 10);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    tool.onMousePress(pressEvent);

    gimp::ToolInputEvent moveEvent;
    moveEvent.canvasPos = QPoint(50, 50);
    moveEvent.buttons = Qt::LeftButton;
    moveEvent.pressure = 1.0F;

    tool.onMouseMove(moveEvent);

    REQUIRE(tool.state() == gimp::ToolState::Active);
}

TEST_CASE("FreeSelectTool returns to Idle after mouse release", "[free_select_tool][unit]")
{
    gimp::FreeSelectTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    tool.setDocument(doc);

    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(10, 10);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    tool.onMousePress(pressEvent);

    // Add more points to form a valid polygon
    gimp::ToolInputEvent moveEvent;
    moveEvent.canvasPos = QPoint(50, 10);
    moveEvent.buttons = Qt::LeftButton;
    moveEvent.pressure = 1.0F;
    tool.onMouseMove(moveEvent);

    moveEvent.canvasPos = QPoint(50, 50);
    tool.onMouseMove(moveEvent);

    gimp::ToolInputEvent releaseEvent;
    releaseEvent.canvasPos = QPoint(10, 50);
    releaseEvent.buttons = Qt::NoButton;
    releaseEvent.pressure = 1.0F;

    tool.onMouseRelease(releaseEvent);

    REQUIRE(tool.state() == gimp::ToolState::Idle);
}

TEST_CASE("FreeSelectTool reset cancels stroke and returns to Idle", "[free_select_tool][unit]")
{
    gimp::FreeSelectTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    tool.setDocument(doc);

    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(10, 10);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    tool.onMousePress(pressEvent);
    REQUIRE(tool.state() == gimp::ToolState::Active);

    tool.reset();
    REQUIRE(tool.state() == gimp::ToolState::Idle);
}

// ============================================================================
// Selection Tests
// ============================================================================

TEST_CASE("FreeSelectTool creates selection on valid polygon", "[free_select_tool][unit]")
{
    gimp::FreeSelectTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    tool.setDocument(doc);

    // Clear any existing selection
    gimp::SelectionManager::instance().setDocument(doc);
    gimp::SelectionManager::instance().clear();
    REQUIRE_FALSE(gimp::SelectionManager::instance().hasSelection());

    // Draw a triangle
    gimp::ToolInputEvent event;
    event.buttons = Qt::LeftButton;
    event.pressure = 1.0F;

    event.canvasPos = QPoint(10, 10);
    tool.onMousePress(event);

    event.canvasPos = QPoint(50, 10);
    tool.onMouseMove(event);

    event.canvasPos = QPoint(30, 50);
    tool.onMouseMove(event);

    event.canvasPos = QPoint(10, 10);
    event.buttons = Qt::NoButton;
    tool.onMouseRelease(event);

    // Should now have a selection
    REQUIRE(gimp::SelectionManager::instance().hasSelection());
}

TEST_CASE("FreeSelectTool does not create selection with fewer than 3 points",
          "[free_select_tool][unit]")
{
    gimp::FreeSelectTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    tool.setDocument(doc);

    // Clear any existing selection
    gimp::SelectionManager::instance().setDocument(doc);
    gimp::SelectionManager::instance().clear();

    // Draw just two points (start + release at same spot)
    gimp::ToolInputEvent event;
    event.buttons = Qt::LeftButton;
    event.pressure = 1.0F;

    event.canvasPos = QPoint(10, 10);
    tool.onMousePress(event);

    event.canvasPos = QPoint(10, 10);
    event.buttons = Qt::NoButton;
    tool.onMouseRelease(event);

    // Should NOT have a selection (need at least 3 points)
    REQUIRE_FALSE(gimp::SelectionManager::instance().hasSelection());
}

TEST_CASE("FreeSelectTool sets preview during stroke", "[free_select_tool][unit]")
{
    gimp::FreeSelectTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    tool.setDocument(doc);

    gimp::SelectionManager::instance().setDocument(doc);
    gimp::SelectionManager::instance().clear();
    gimp::SelectionManager::instance().clearPreview();

    gimp::ToolInputEvent event;
    event.buttons = Qt::LeftButton;
    event.pressure = 1.0F;

    event.canvasPos = QPoint(10, 10);
    tool.onMousePress(event);

    // Move to create a path with actual line segments (single point is not a valid preview)
    event.canvasPos = QPoint(50, 50);
    tool.onMouseMove(event);

    // Preview should be set after we have at least 2 points forming a line
    REQUIRE(gimp::SelectionManager::instance().hasPreview());
}

TEST_CASE("FreeSelectTool clears preview after stroke", "[free_select_tool][unit]")
{
    gimp::FreeSelectTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    tool.setDocument(doc);

    gimp::SelectionManager::instance().setDocument(doc);
    gimp::SelectionManager::instance().clear();

    // Draw a valid polygon
    gimp::ToolInputEvent event;
    event.buttons = Qt::LeftButton;
    event.pressure = 1.0F;

    event.canvasPos = QPoint(10, 10);
    tool.onMousePress(event);

    event.canvasPos = QPoint(50, 10);
    tool.onMouseMove(event);

    event.canvasPos = QPoint(30, 50);
    tool.onMouseMove(event);

    event.canvasPos = QPoint(10, 10);
    event.buttons = Qt::NoButton;
    tool.onMouseRelease(event);

    // Preview should be cleared after release
    REQUIRE_FALSE(gimp::SelectionManager::instance().hasPreview());
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE("FreeSelectTool handles empty document gracefully", "[free_select_tool][unit]")
{
    gimp::FreeSelectTool tool;

    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(50, 50);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    REQUIRE_NOTHROW(tool.onMousePress(pressEvent));
}

TEST_CASE("FreeSelectTool handles document with no layers", "[free_select_tool][unit]")
{
    gimp::FreeSelectTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    tool.setDocument(doc);

    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(50, 50);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    REQUIRE_NOTHROW(tool.onMousePress(pressEvent));
}
