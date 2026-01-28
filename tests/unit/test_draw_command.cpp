/**
 * @file test_draw_command.cpp
 * @brief Unit tests for DrawCommand undo/redo functionality.
 * @author Laurent Jiang
 * @date 2026-01-27
 */

#include "core/commands/draw_command.h"
#include "core/layer.h"

#include <catch2/catch_test_macros.hpp>

namespace {

/**
 * @brief Helper to create a layer with test data.
 */
std::shared_ptr<gimp::Layer> createTestLayer(int width, int height)
{
    return std::make_shared<gimp::Layer>(width, height);
}

/**
 * @brief Helper to set a rectangular region to a specific RGBA color.
 */
void setRegionColor(std::shared_ptr<gimp::Layer> layer,
                    int x,
                    int y,
                    int width,
                    int height,
                    uint8_t r,
                    uint8_t g,
                    uint8_t b,
                    uint8_t a)
{
    auto& data = layer->data();
    const int layerWidth = layer->width();
    const int pixelSize = 4;

    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            const int dstRow = y + row;
            const int dstCol = x + col;

            // Bounds check
            if (dstRow < 0 || dstRow >= layer->height() || dstCol < 0 || dstCol >= layer->width()) {
                continue;
            }

            const int offset = (dstRow * layerWidth + dstCol) * pixelSize;
            data[offset + 0] = r;
            data[offset + 1] = g;
            data[offset + 2] = b;
            data[offset + 3] = a;
        }
    }
}

/**
 * @brief Helper to get the color at a specific pixel.
 */
void getPixelColor(const std::shared_ptr<gimp::Layer>& layer,
                   int x,
                   int y,
                   uint8_t& r,
                   uint8_t& g,
                   uint8_t& b,
                   uint8_t& a)
{
    const auto& data = layer->data();
    const int layerWidth = layer->width();
    const int pixelSize = 4;

    if (x < 0 || x >= layer->width() || y < 0 || y >= layer->height()) {
        r = g = b = a = 0;
        return;
    }

    const int offset = (y * layerWidth + x) * pixelSize;
    r = data[offset + 0];
    g = data[offset + 1];
    b = data[offset + 2];
    a = data[offset + 3];
}

/**
 * @brief Helper to verify a region has a specific color.
 */
bool regionHasColor(const std::shared_ptr<gimp::Layer>& layer,
                    int x,
                    int y,
                    int width,
                    int height,
                    uint8_t r,
                    uint8_t g,
                    uint8_t b,
                    uint8_t a)
{
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            uint8_t pr, pg, pb, pa;
            getPixelColor(layer, x + col, y + row, pr, pg, pb, pa);

            if (pr != r || pg != g || pb != b || pa != a) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace

TEST_CASE("DrawCommand construction", "[draw_command][unit]")
{
    auto layer = createTestLayer(100, 100);
    auto cmd = std::make_shared<gimp::DrawCommand>(layer, 10, 20, 30, 40);

    REQUIRE(cmd != nullptr);
}

TEST_CASE("DrawCommand captures before state", "[draw_command][unit]")
{
    auto layer = createTestLayer(100, 100);

    // Set initial color
    setRegionColor(layer, 10, 20, 30, 40, 255, 0, 0, 255);

    auto cmd = std::make_shared<gimp::DrawCommand>(layer, 10, 20, 30, 40);
    cmd->captureBeforeState();

    // Verify the region still has the original color
    REQUIRE(regionHasColor(layer, 10, 20, 30, 40, 255, 0, 0, 255));

    // Modify the layer
    setRegionColor(layer, 10, 20, 30, 40, 0, 255, 0, 255);

    // Verify region is now green
    REQUIRE(regionHasColor(layer, 10, 20, 30, 40, 0, 255, 0, 255));

    // After undo, should be red again
    cmd->undo();
    REQUIRE(regionHasColor(layer, 10, 20, 30, 40, 255, 0, 0, 255));
}

TEST_CASE("DrawCommand captures after state", "[draw_command][unit]")
{
    auto layer = createTestLayer(100, 100);

    // Set initial color
    setRegionColor(layer, 10, 20, 30, 40, 255, 0, 0, 255);

    auto cmd = std::make_shared<gimp::DrawCommand>(layer, 10, 20, 30, 40);
    cmd->captureBeforeState();

    // Simulate drawing: change to green
    setRegionColor(layer, 10, 20, 30, 40, 0, 255, 0, 255);
    cmd->captureAfterState();

    // Change to blue to test apply
    setRegionColor(layer, 10, 20, 30, 40, 0, 0, 255, 255);
    REQUIRE(regionHasColor(layer, 10, 20, 30, 40, 0, 0, 255, 255));

    // Apply should restore to green (the after state)
    cmd->apply();
    REQUIRE(regionHasColor(layer, 10, 20, 30, 40, 0, 255, 0, 255));
}

TEST_CASE("DrawCommand undo restores before state", "[draw_command][unit]")
{
    auto layer = createTestLayer(100, 100);

    // Initial: Red
    setRegionColor(layer, 10, 20, 30, 40, 255, 0, 0, 255);
    auto cmd = std::make_shared<gimp::DrawCommand>(layer, 10, 20, 30, 40);
    cmd->captureBeforeState();

    // Draw: Green
    setRegionColor(layer, 10, 20, 30, 40, 0, 255, 0, 255);
    cmd->captureAfterState();

    // Undo should go back to red
    cmd->undo();
    REQUIRE(regionHasColor(layer, 10, 20, 30, 40, 255, 0, 0, 255));
}

TEST_CASE("DrawCommand apply restores after state", "[draw_command][unit]")
{
    auto layer = createTestLayer(100, 100);

    // Initial: Red
    setRegionColor(layer, 10, 20, 30, 40, 255, 0, 0, 255);
    auto cmd = std::make_shared<gimp::DrawCommand>(layer, 10, 20, 30, 40);
    cmd->captureBeforeState();

    // Draw: Green
    setRegionColor(layer, 10, 20, 30, 40, 0, 255, 0, 255);
    cmd->captureAfterState();

    // Undo to red
    cmd->undo();
    REQUIRE(regionHasColor(layer, 10, 20, 30, 40, 255, 0, 0, 255));

    // Apply should go back to green
    cmd->apply();
    REQUIRE(regionHasColor(layer, 10, 20, 30, 40, 0, 255, 0, 255));
}

TEST_CASE("DrawCommand handles multiple undo/redo cycles", "[draw_command][unit]")
{
    auto layer = createTestLayer(100, 100);

    // Initial: Red
    setRegionColor(layer, 10, 20, 30, 40, 255, 0, 0, 255);
    auto cmd = std::make_shared<gimp::DrawCommand>(layer, 10, 20, 30, 40);
    cmd->captureBeforeState();

    // Draw: Green
    setRegionColor(layer, 10, 20, 30, 40, 0, 255, 0, 255);
    cmd->captureAfterState();

    // First cycle
    cmd->undo();
    REQUIRE(regionHasColor(layer, 10, 20, 30, 40, 255, 0, 0, 255));

    cmd->apply();
    REQUIRE(regionHasColor(layer, 10, 20, 30, 40, 0, 255, 0, 255));

    // Second cycle
    cmd->undo();
    REQUIRE(regionHasColor(layer, 10, 20, 30, 40, 255, 0, 0, 255));

    cmd->apply();
    REQUIRE(regionHasColor(layer, 10, 20, 30, 40, 0, 255, 0, 255));
}

TEST_CASE("DrawCommand handles partial region (clipping)", "[draw_command][unit]")
{
    auto layer = createTestLayer(100, 100);

    // Set background to black
    setRegionColor(layer, 0, 0, 100, 100, 0, 0, 0, 255);

    // Draw command with region extending beyond layer bounds
    auto cmd = std::make_shared<gimp::DrawCommand>(layer, 80, 80, 50, 50);
    cmd->captureBeforeState();

    // Color the valid region (80,80) to (99,99)
    setRegionColor(layer, 80, 80, 20, 20, 255, 0, 0, 255);
    cmd->captureAfterState();

    // Undo should restore to black
    cmd->undo();
    REQUIRE(regionHasColor(layer, 80, 80, 20, 20, 0, 0, 0, 255));

    // Apply should restore to red
    cmd->apply();
    REQUIRE(regionHasColor(layer, 80, 80, 20, 20, 255, 0, 0, 255));
}

TEST_CASE("DrawCommand doesn't affect regions outside affected area", "[draw_command][unit]")
{
    auto layer = createTestLayer(100, 100);

    // Set background to black
    setRegionColor(layer, 0, 0, 100, 100, 0, 0, 0, 255);

    auto cmd = std::make_shared<gimp::DrawCommand>(layer, 10, 10, 30, 30);
    cmd->captureBeforeState();

    // Change the affected region to red
    setRegionColor(layer, 10, 10, 30, 30, 255, 0, 0, 255);
    cmd->captureAfterState();

    // Change area outside the region to white
    setRegionColor(layer, 50, 50, 30, 30, 255, 255, 255, 255);

    // Undo should only affect the command region, leaving white area unchanged
    cmd->undo();
    REQUIRE(regionHasColor(layer, 10, 10, 30, 30, 0, 0, 0, 255));
    REQUIRE(regionHasColor(layer, 50, 50, 30, 30, 255, 255, 255, 255));
}

TEST_CASE("DrawCommand with zero-sized region doesn't crash", "[draw_command][unit]")
{
    auto layer = createTestLayer(100, 100);

    // Create command with zero width (completely out of bounds)
    auto cmd = std::make_shared<gimp::DrawCommand>(layer, -50, -50, 10, 10);
    cmd->captureBeforeState();
    cmd->captureAfterState();

    // Should not crash
    cmd->undo();
    cmd->apply();

    REQUIRE(true);
}

TEST_CASE("DrawCommand with null layer doesn't crash", "[draw_command][unit]")
{
    // This test verifies robustness when layer is null
    auto cmd = std::make_shared<gimp::DrawCommand>(nullptr, 10, 10, 30, 30);

    // Should not crash even with null layer
    cmd->captureBeforeState();
    cmd->captureAfterState();
    cmd->undo();
    cmd->apply();

    REQUIRE(true);
}

TEST_CASE("DrawCommand preserves unaffected pixels", "[draw_command][unit]")
{
    auto layer = createTestLayer(100, 100);

    // Create a gradient: red on left, blue on right
    setRegionColor(layer, 0, 0, 50, 100, 255, 0, 0, 255);   // Left: red
    setRegionColor(layer, 50, 0, 50, 100, 0, 0, 255, 255);  // Right: blue

    // Command affects only the left side
    auto cmd = std::make_shared<gimp::DrawCommand>(layer, 0, 0, 50, 100);
    cmd->captureBeforeState();

    // Draw green on the left
    setRegionColor(layer, 0, 0, 50, 100, 0, 255, 0, 255);
    cmd->captureAfterState();

    // Verify left is green
    REQUIRE(regionHasColor(layer, 0, 0, 50, 100, 0, 255, 0, 255));

    // Right should still be blue
    REQUIRE(regionHasColor(layer, 50, 0, 50, 100, 0, 0, 255, 255));

    // Undo
    cmd->undo();

    // Left should be red again
    REQUIRE(regionHasColor(layer, 0, 0, 50, 100, 255, 0, 0, 255));

    // Right should still be blue (unchanged by the command)
    REQUIRE(regionHasColor(layer, 50, 0, 50, 100, 0, 0, 255, 255));
}

TEST_CASE("DrawCommand handles different alpha values", "[draw_command][unit]")
{
    auto layer = createTestLayer(100, 100);

    // Initial: fully opaque red
    setRegionColor(layer, 10, 10, 30, 30, 255, 0, 0, 255);
    auto cmd = std::make_shared<gimp::DrawCommand>(layer, 10, 10, 30, 30);
    cmd->captureBeforeState();

    // Draw: semi-transparent green (alpha = 128)
    setRegionColor(layer, 10, 10, 30, 30, 0, 255, 0, 128);
    cmd->captureAfterState();

    // Undo
    cmd->undo();
    uint8_t r, g, b, a;
    getPixelColor(layer, 10, 10, r, g, b, a);
    REQUIRE(r == 255);
    REQUIRE(g == 0);
    REQUIRE(b == 0);
    REQUIRE(a == 255);

    // Apply
    cmd->apply();
    getPixelColor(layer, 10, 10, r, g, b, a);
    REQUIRE(r == 0);
    REQUIRE(g == 255);
    REQUIRE(b == 0);
    REQUIRE(a == 128);
}

TEST_CASE("DrawCommand with single pixel region", "[draw_command][unit]")
{
    auto layer = createTestLayer(100, 100);

    // Set a single pixel to red
    setRegionColor(layer, 50, 50, 1, 1, 255, 0, 0, 255);
    auto cmd = std::make_shared<gimp::DrawCommand>(layer, 50, 50, 1, 1);
    cmd->captureBeforeState();

    // Change to green
    setRegionColor(layer, 50, 50, 1, 1, 0, 255, 0, 255);
    cmd->captureAfterState();

    // Undo
    cmd->undo();
    uint8_t r, g, b, a;
    getPixelColor(layer, 50, 50, r, g, b, a);
    REQUIRE(r == 255);
    REQUIRE(g == 0);
    REQUIRE(b == 0);
    REQUIRE(a == 255);

    // Apply
    cmd->apply();
    getPixelColor(layer, 50, 50, r, g, b, a);
    REQUIRE(r == 0);
    REQUIRE(g == 255);
    REQUIRE(b == 0);
    REQUIRE(a == 255);
}

TEST_CASE("DrawCommand with full-size region", "[draw_command][unit]")
{
    auto layer = createTestLayer(100, 100);

    // Set entire layer to red
    setRegionColor(layer, 0, 0, 100, 100, 255, 0, 0, 255);
    auto cmd = std::make_shared<gimp::DrawCommand>(layer, 0, 0, 100, 100);
    cmd->captureBeforeState();

    // Change entire layer to green
    setRegionColor(layer, 0, 0, 100, 100, 0, 255, 0, 255);
    cmd->captureAfterState();

    // Undo should restore to red
    cmd->undo();
    REQUIRE(regionHasColor(layer, 0, 0, 100, 100, 255, 0, 0, 255));

    // Apply should restore to green
    cmd->apply();
    REQUIRE(regionHasColor(layer, 0, 0, 100, 100, 0, 255, 0, 255));
}

TEST_CASE("DrawCommand sequence: multiple draws", "[draw_command][unit]")
{
    auto layer = createTestLayer(100, 100);

    // Initialize to black
    setRegionColor(layer, 0, 0, 100, 100, 0, 0, 0, 255);

    // First draw: red square at (10,10)
    auto cmd1 = std::make_shared<gimp::DrawCommand>(layer, 10, 10, 20, 20);
    cmd1->captureBeforeState();
    setRegionColor(layer, 10, 10, 20, 20, 255, 0, 0, 255);
    cmd1->captureAfterState();

    // Second draw: green square at (50,50)
    auto cmd2 = std::make_shared<gimp::DrawCommand>(layer, 50, 50, 20, 20);
    cmd2->captureBeforeState();
    setRegionColor(layer, 50, 50, 20, 20, 0, 255, 0, 255);
    cmd2->captureAfterState();

    // Verify current state: red and green squares
    REQUIRE(regionHasColor(layer, 10, 10, 20, 20, 255, 0, 0, 255));
    REQUIRE(regionHasColor(layer, 50, 50, 20, 20, 0, 255, 0, 255));

    // Undo second command
    cmd2->undo();
    REQUIRE(regionHasColor(layer, 10, 10, 20, 20, 255, 0, 0, 255));
    REQUIRE(regionHasColor(layer, 50, 50, 20, 20, 0, 0, 0, 255));

    // Undo first command
    cmd1->undo();
    REQUIRE(regionHasColor(layer, 10, 10, 20, 20, 0, 0, 0, 255));
    REQUIRE(regionHasColor(layer, 50, 50, 20, 20, 0, 0, 0, 255));

    // Redo first command
    cmd1->apply();
    REQUIRE(regionHasColor(layer, 10, 10, 20, 20, 255, 0, 0, 255));
    REQUIRE(regionHasColor(layer, 50, 50, 20, 20, 0, 0, 0, 255));

    // Redo second command
    cmd2->apply();
    REQUIRE(regionHasColor(layer, 10, 10, 20, 20, 255, 0, 0, 255));
    REQUIRE(regionHasColor(layer, 50, 50, 20, 20, 0, 255, 0, 255));
}
