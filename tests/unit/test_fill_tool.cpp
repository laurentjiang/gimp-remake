/**
 * @file test_fill_tool.cpp
 * @brief Unit tests for FillTool (bucket fill with flood-fill algorithm).
 * @author Laurent Jiang
 * @date 2025-06-28
 */

#include "core/command_bus.h"
#include "core/layer.h"
#include "core/tool_factory.h"
#include "core/tools/fill_tool.h"
#include "io/project_file.h"

#include <catch2/catch_test_macros.hpp>

// ============================================================================
// Basic Property Tests
// ============================================================================

TEST_CASE("FillTool has correct id", "[fill_tool][unit]")
{
    gimp::FillTool tool;
    REQUIRE(tool.id() == "bucket_fill");
}

TEST_CASE("FillTool has correct name", "[fill_tool][unit]")
{
    gimp::FillTool tool;
    REQUIRE(tool.name() == "Bucket Fill");
}

TEST_CASE("FillTool default tolerance is 0", "[fill_tool][unit]")
{
    gimp::FillTool tool;
    REQUIRE(tool.tolerance() == 0);
}

TEST_CASE("FillTool setTolerance updates tolerance", "[fill_tool][unit]")
{
    gimp::FillTool tool;

    tool.setTolerance(50);
    REQUIRE(tool.tolerance() == 50);

    tool.setTolerance(0);
    REQUIRE(tool.tolerance() == 0);

    tool.setTolerance(255);
    REQUIRE(tool.tolerance() == 255);
}

TEST_CASE("FillTool setTolerance clamps to valid range", "[fill_tool][unit]")
{
    gimp::FillTool tool;

    // Negative should clamp to 0
    tool.setTolerance(-10);
    REQUIRE(tool.tolerance() == 0);

    // Above 255 should clamp to 255
    tool.setTolerance(300);
    REQUIRE(tool.tolerance() == 255);
}

// ============================================================================
// State Machine Tests
// ============================================================================

TEST_CASE("FillTool starts in Idle state", "[fill_tool][unit]")
{
    gimp::FillTool tool;
    REQUIRE(tool.state() == gimp::ToolState::Idle);
}

TEST_CASE("FillTool returns to Idle after fill operation", "[fill_tool][unit]")
{
    gimp::FillTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(10, 10);
    doc->addLayer();
    tool.setDocument(doc);
    gimp::ToolFactory::instance().setForegroundColor(0xFF0000FF);  // Red

    gimp::ToolInputEvent pressEvent;
    pressEvent.canvasPos = QPoint(5, 5);
    pressEvent.buttons = Qt::LeftButton;
    pressEvent.pressure = 1.0F;

    tool.onMousePress(pressEvent);

    gimp::ToolInputEvent releaseEvent;
    releaseEvent.canvasPos = QPoint(5, 5);
    releaseEvent.buttons = Qt::NoButton;
    releaseEvent.pressure = 1.0F;

    tool.onMouseRelease(releaseEvent);

    REQUIRE(tool.state() == gimp::ToolState::Idle);
}

TEST_CASE("FillTool reset returns to Idle", "[fill_tool][unit]")
{
    gimp::FillTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(10, 10);
    doc->addLayer();
    tool.setDocument(doc);

    tool.reset();
    REQUIRE(tool.state() == gimp::ToolState::Idle);
}

// ============================================================================
// Flood Fill Functionality Tests
// ============================================================================

TEST_CASE("FillTool fills uniform region with new color", "[fill_tool][unit]")
{
    gimp::FillTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(10, 10);
    doc->addLayer();
    tool.setDocument(doc);

    // Layer starts as white (255, 255, 255, 255)
    auto layer = doc->layers()[0];
    auto& data = layer->data();

    // Set foreground to red
    gimp::ToolFactory::instance().setForegroundColor(0xFF0000FF);  // RGBA: Red

    // Click in the center
    gimp::ToolInputEvent event;
    event.canvasPos = QPoint(5, 5);
    event.buttons = Qt::LeftButton;
    event.pressure = 1.0F;

    tool.onMousePress(event);

    gimp::ToolInputEvent releaseEvent;
    releaseEvent.canvasPos = QPoint(5, 5);
    releaseEvent.buttons = Qt::NoButton;
    releaseEvent.pressure = 1.0F;

    tool.onMouseRelease(releaseEvent);

    // All pixels should now be red
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            int idx = (y * 10 + x) * 4;
            REQUIRE(data[idx] == 255);      // R
            REQUIRE(data[idx + 1] == 0);    // G
            REQUIRE(data[idx + 2] == 0);    // B
            REQUIRE(data[idx + 3] == 255);  // A
        }
    }
}

TEST_CASE("FillTool respects region boundaries", "[fill_tool][unit]")
{
    gimp::FillTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(10, 10);
    doc->addLayer();
    tool.setDocument(doc);

    auto layer = doc->layers()[0];
    auto& data = layer->data();

    // Initialize entire layer to white (layers start as transparent black)
    for (size_t i = 0; i < data.size(); i += 4) {
        data[i] = 255;      // R
        data[i + 1] = 255;  // G
        data[i + 2] = 255;  // B
        data[i + 3] = 255;  // A
    }

    // Draw a black vertical line at x=5 to divide the canvas
    for (int y = 0; y < 10; ++y) {
        int idx = (y * 10 + 5) * 4;
        data[idx] = 0;        // R
        data[idx + 1] = 0;    // G
        data[idx + 2] = 0;    // B
        data[idx + 3] = 255;  // A
    }

    // Set foreground to red
    gimp::ToolFactory::instance().setForegroundColor(0xFF0000FF);

    // Click on the left side (x=2)
    gimp::ToolInputEvent event;
    event.canvasPos = QPoint(2, 5);
    event.buttons = Qt::LeftButton;
    event.pressure = 1.0F;

    tool.onMousePress(event);

    gimp::ToolInputEvent releaseEvent;
    releaseEvent.canvasPos = QPoint(2, 5);
    releaseEvent.buttons = Qt::NoButton;
    releaseEvent.pressure = 1.0F;

    tool.onMouseRelease(releaseEvent);

    // Left side (x < 5) should be red
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 5; ++x) {
            int idx = (y * 10 + x) * 4;
            REQUIRE(data[idx] == 255);    // R
            REQUIRE(data[idx + 1] == 0);  // G
            REQUIRE(data[idx + 2] == 0);  // B
        }
    }

    // Black line at x=5 should remain black
    for (int y = 0; y < 10; ++y) {
        int idx = (y * 10 + 5) * 4;
        REQUIRE(data[idx] == 0);      // R
        REQUIRE(data[idx + 1] == 0);  // G
        REQUIRE(data[idx + 2] == 0);  // B
    }

    // Right side (x > 5) should remain white
    for (int y = 0; y < 10; ++y) {
        for (int x = 6; x < 10; ++x) {
            int idx = (y * 10 + x) * 4;
            REQUIRE(data[idx] == 255);      // R
            REQUIRE(data[idx + 1] == 255);  // G
            REQUIRE(data[idx + 2] == 255);  // B
        }
    }
}

TEST_CASE("FillTool with tolerance fills similar colors", "[fill_tool][unit]")
{
    gimp::FillTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(10, 10);
    doc->addLayer();
    tool.setDocument(doc);

    auto layer = doc->layers()[0];
    auto& data = layer->data();

    // Initialize right half to white (255, 255, 255, 255)
    for (int y = 0; y < 10; ++y) {
        for (int x = 5; x < 10; ++x) {
            int idx = (y * 10 + x) * 4;
            data[idx] = 255;
            data[idx + 1] = 255;
            data[idx + 2] = 255;
            data[idx + 3] = 255;
        }
    }

    // Fill left half with slightly off-white (close to white)
    // Left half: RGB(250, 250, 250)
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 5; ++x) {
            int idx = (y * 10 + x) * 4;
            data[idx] = 250;
            data[idx + 1] = 250;
            data[idx + 2] = 250;
            data[idx + 3] = 255;
        }
    }

    // Set tolerance to 10 (should match both shades)
    tool.setTolerance(10);
    gimp::ToolFactory::instance().setForegroundColor(0x0000FFFF);  // Blue

    // Click on left side
    gimp::ToolInputEvent event;
    event.canvasPos = QPoint(2, 5);
    event.buttons = Qt::LeftButton;
    event.pressure = 1.0F;

    tool.onMousePress(event);

    gimp::ToolInputEvent releaseEvent;
    releaseEvent.canvasPos = QPoint(2, 5);
    releaseEvent.buttons = Qt::NoButton;
    releaseEvent.pressure = 1.0F;

    tool.onMouseRelease(releaseEvent);

    // With tolerance 10, both left (250,250,250) and right (255,255,255) should be filled
    // because |255-250| = 5 < 10 tolerance
    for (int x = 0; x < 10; ++x) {
        int idx = (5 * 10 + x) * 4;
        REQUIRE(data[idx] == 0);        // R (blue has R=0)
        REQUIRE(data[idx + 1] == 0);    // G
        REQUIRE(data[idx + 2] == 255);  // B
    }
}

TEST_CASE("FillTool with zero tolerance only fills exact matches", "[fill_tool][unit]")
{
    gimp::FillTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(10, 10);
    doc->addLayer();
    tool.setDocument(doc);

    auto layer = doc->layers()[0];
    auto& data = layer->data();

    // Initialize right half to pure white (255, 255, 255, 255)
    for (int y = 0; y < 10; ++y) {
        for (int x = 5; x < 10; ++x) {
            int idx = (y * 10 + x) * 4;
            data[idx] = 255;
            data[idx + 1] = 255;
            data[idx + 2] = 255;
            data[idx + 3] = 255;
        }
    }

    // Left half: RGB(254, 255, 255) - slightly different from white
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 5; ++x) {
            int idx = (y * 10 + x) * 4;
            data[idx] = 254;
            data[idx + 1] = 255;
            data[idx + 2] = 255;
            data[idx + 3] = 255;
        }
    }

    // Set tolerance to 0 (exact match only)
    tool.setTolerance(0);
    gimp::ToolFactory::instance().setForegroundColor(0x00FF00FF);  // Green

    // Click on right side (pure white)
    gimp::ToolInputEvent event;
    event.canvasPos = QPoint(7, 5);
    event.buttons = Qt::LeftButton;
    event.pressure = 1.0F;

    tool.onMousePress(event);

    gimp::ToolInputEvent releaseEvent;
    releaseEvent.canvasPos = QPoint(7, 5);
    releaseEvent.buttons = Qt::NoButton;
    releaseEvent.pressure = 1.0F;

    tool.onMouseRelease(releaseEvent);

    // Right side (pure white) should be green
    for (int y = 0; y < 10; ++y) {
        for (int x = 5; x < 10; ++x) {
            int idx = (y * 10 + x) * 4;
            REQUIRE(data[idx] == 0);        // R
            REQUIRE(data[idx + 1] == 255);  // G
            REQUIRE(data[idx + 2] == 0);    // B
        }
    }

    // Left side (slightly off-white) should NOT be filled
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 5; ++x) {
            int idx = (y * 10 + x) * 4;
            REQUIRE(data[idx] == 254);      // R (unchanged)
            REQUIRE(data[idx + 1] == 255);  // G (unchanged)
            REQUIRE(data[idx + 2] == 255);  // B (unchanged)
        }
    }
}

TEST_CASE("FillTool does not fill when clicking on target color", "[fill_tool][unit]")
{
    gimp::FillTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(10, 10);
    doc->addLayer();
    tool.setDocument(doc);

    auto layer = doc->layers()[0];
    auto& data = layer->data();

    // Fill entire layer with red
    for (size_t i = 0; i < data.size(); i += 4) {
        data[i] = 255;
        data[i + 1] = 0;
        data[i + 2] = 0;
        data[i + 3] = 255;
    }

    // Set foreground to red (same as existing color)
    gimp::ToolFactory::instance().setForegroundColor(0xFF0000FF);

    // Click
    gimp::ToolInputEvent event;
    event.canvasPos = QPoint(5, 5);
    event.buttons = Qt::LeftButton;
    event.pressure = 1.0F;

    tool.onMousePress(event);

    gimp::ToolInputEvent releaseEvent;
    releaseEvent.canvasPos = QPoint(5, 5);
    releaseEvent.buttons = Qt::NoButton;
    releaseEvent.pressure = 1.0F;

    // Should not crash or infinite loop when target == source
    REQUIRE_NOTHROW(tool.onMouseRelease(releaseEvent));

    // Pixels should remain red
    int idx = (5 * 10 + 5) * 4;
    REQUIRE(data[idx] == 255);    // R
    REQUIRE(data[idx + 1] == 0);  // G
    REQUIRE(data[idx + 2] == 0);  // B
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_CASE("FillTool handles empty document gracefully", "[fill_tool][unit]")
{
    gimp::FillTool tool;

    // No document set
    gimp::ToolInputEvent event;
    event.canvasPos = QPoint(5, 5);
    event.buttons = Qt::LeftButton;
    event.pressure = 1.0F;

    REQUIRE_NOTHROW(tool.onMousePress(event));
}

TEST_CASE("FillTool handles document with no layers", "[fill_tool][unit]")
{
    gimp::FillTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(10, 10);
    tool.setDocument(doc);

    gimp::ToolInputEvent event;
    event.canvasPos = QPoint(5, 5);
    event.buttons = Qt::LeftButton;
    event.pressure = 1.0F;

    REQUIRE_NOTHROW(tool.onMousePress(event));
}

TEST_CASE("FillTool handles click outside canvas bounds", "[fill_tool][unit]")
{
    gimp::FillTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(10, 10);
    doc->addLayer();
    tool.setDocument(doc);
    gimp::ToolFactory::instance().setForegroundColor(0xFF0000FF);

    // Click outside bounds
    gimp::ToolInputEvent event;
    event.canvasPos = QPoint(-5, -5);
    event.buttons = Qt::LeftButton;
    event.pressure = 1.0F;

    REQUIRE_NOTHROW(tool.onMousePress(event));

    // Click far outside
    event.canvasPos = QPoint(100, 100);
    REQUIRE_NOTHROW(tool.onMousePress(event));
}

TEST_CASE("FillTool handles single pixel canvas", "[fill_tool][unit]")
{
    gimp::FillTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(1, 1);
    doc->addLayer();
    tool.setDocument(doc);
    gimp::ToolFactory::instance().setForegroundColor(0x00FF00FF);  // Green

    auto layer = doc->layers()[0];
    auto& data = layer->data();

    gimp::ToolInputEvent event;
    event.canvasPos = QPoint(0, 0);
    event.buttons = Qt::LeftButton;
    event.pressure = 1.0F;

    tool.onMousePress(event);

    gimp::ToolInputEvent releaseEvent;
    releaseEvent.canvasPos = QPoint(0, 0);
    releaseEvent.buttons = Qt::NoButton;
    releaseEvent.pressure = 1.0F;

    tool.onMouseRelease(releaseEvent);

    REQUIRE(data[0] == 0);    // R
    REQUIRE(data[1] == 255);  // G
    REQUIRE(data[2] == 0);    // B
    REQUIRE(data[3] == 255);  // A
}

TEST_CASE("FillTool handles right mouse button (no action)", "[fill_tool][unit]")
{
    gimp::FillTool tool;
    auto doc = std::make_shared<gimp::ProjectFile>(10, 10);
    doc->addLayer();
    tool.setDocument(doc);
    gimp::ToolFactory::instance().setForegroundColor(0xFF0000FF);

    auto layer = doc->layers()[0];
    auto& data = layer->data();

    // Store original color
    uint8_t origR = data[0];
    uint8_t origG = data[1];
    uint8_t origB = data[2];

    // Right click
    gimp::ToolInputEvent event;
    event.canvasPos = QPoint(0, 0);
    event.buttons = Qt::RightButton;
    event.pressure = 1.0F;

    tool.onMousePress(event);

    // Should not fill
    REQUIRE(data[0] == origR);
    REQUIRE(data[1] == origG);
    REQUIRE(data[2] == origB);
}
