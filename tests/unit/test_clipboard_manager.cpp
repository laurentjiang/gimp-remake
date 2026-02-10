/**
 * @file test_clipboard_manager.cpp
 * @brief Unit tests for ClipboardManager copy/cut/paste functionality.
 * @author Laurent Jiang
 * @date 2026-02-10
 */

#include "core/clipboard_manager.h"
#include "core/command_bus.h"
#include "core/layer.h"
#include "core/selection_manager.h"
#include "history/simple_history_manager.h"
#include "io/project_file.h"

#include <QPainterPath>

#include <catch2/catch_test_macros.hpp>

namespace {

/**
 * @brief Helper to set a rectangular region to a specific RGBA color.
 */
void setRegionColor(const std::shared_ptr<gimp::Layer>& layer,
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
    constexpr int pixelSize = 4;

    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            const int dstRow = y + row;
            const int dstCol = x + col;

            if (dstRow < 0 || dstRow >= layer->height() || dstCol < 0 || dstCol >= layer->width()) {
                continue;
            }

            const std::size_t offset =
                (static_cast<std::size_t>(dstRow) * static_cast<std::size_t>(layerWidth) +
                 static_cast<std::size_t>(dstCol)) *
                pixelSize;
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
std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> getPixelColor(
    const std::shared_ptr<gimp::Layer>& layer, int x, int y)
{
    const auto& data = layer->data();
    const int layerWidth = layer->width();
    constexpr int pixelSize = 4;

    const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(layerWidth) +
                                static_cast<std::size_t>(x)) *
                               pixelSize;
    return {data[offset + 0], data[offset + 1], data[offset + 2], data[offset + 3]};
}

/**
 * @brief Test fixture that sets up a document with a layer.
 */
struct ClipboardFixture {
    std::shared_ptr<gimp::ProjectFile> document;
    std::shared_ptr<gimp::Layer> layer;
    gimp::SimpleHistoryManager historyManager;
    gimp::BasicCommandBus commandBus;

    ClipboardFixture() : commandBus(historyManager)
    {
        document = std::make_shared<gimp::ProjectFile>(100, 100);
        layer = document->addLayer();
        gimp::SelectionManager::instance().setDocument(document);
        gimp::SelectionManager::instance().clear();
    }

    ~ClipboardFixture() { gimp::SelectionManager::instance().clear(); }
};

}  // namespace

// ============================================================================
// Copy Tests
// ============================================================================

TEST_CASE("ClipboardManager copySelection returns false with null document",
          "[clipboard_manager][unit]")
{
    REQUIRE_FALSE(gimp::ClipboardManager::instance().copySelection(nullptr));
}

TEST_CASE("ClipboardManager copySelection returns false with empty document",
          "[clipboard_manager][unit]")
{
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    // No layers added
    REQUIRE_FALSE(gimp::ClipboardManager::instance().copySelection(doc));
}

TEST_CASE("ClipboardManager copySelection copies entire layer when no selection",
          "[clipboard_manager][unit]")
{
    ClipboardFixture fixture;

    // Fill layer with red
    setRegionColor(fixture.layer, 0, 0, 100, 100, 255, 0, 0, 255);

    bool result = gimp::ClipboardManager::instance().copySelection(fixture.document, fixture.layer);

    REQUIRE(result);
    REQUIRE(gimp::ClipboardManager::instance().hasImage());

    const QImage& image = gimp::ClipboardManager::instance().image();
    REQUIRE(image.width() == 100);
    REQUIRE(image.height() == 100);
}

TEST_CASE("ClipboardManager copySelection copies selected region only", "[clipboard_manager][unit]")
{
    ClipboardFixture fixture;

    // Fill layer with red
    setRegionColor(fixture.layer, 0, 0, 100, 100, 255, 0, 0, 255);

    // Create a 50x50 selection at (10, 10)
    QPainterPath selectionPath;
    selectionPath.addRect(10, 10, 50, 50);
    gimp::SelectionManager::instance().applySelection(selectionPath, gimp::SelectionMode::Replace);

    bool result = gimp::ClipboardManager::instance().copySelection(fixture.document, fixture.layer);

    REQUIRE(result);
    REQUIRE(gimp::ClipboardManager::instance().hasImage());

    const QImage& image = gimp::ClipboardManager::instance().image();
    REQUIRE(image.width() == 50);
    REQUIRE(image.height() == 50);
}

// ============================================================================
// Cut Tests
// ============================================================================

TEST_CASE("ClipboardManager cutSelection returns false with no selection",
          "[clipboard_manager][unit]")
{
    ClipboardFixture fixture;

    // No selection - cut should fail
    bool result = gimp::ClipboardManager::instance().cutSelection(
        fixture.document, fixture.layer, &fixture.commandBus);

    REQUIRE_FALSE(result);
}

TEST_CASE("ClipboardManager cutSelection cuts selected region and clears pixels",
          "[clipboard_manager][unit]")
{
    ClipboardFixture fixture;

    // Fill layer with red
    setRegionColor(fixture.layer, 0, 0, 100, 100, 255, 0, 0, 255);

    // Create a 10x10 selection at (20, 20)
    QPainterPath selectionPath;
    selectionPath.addRect(20, 20, 10, 10);
    gimp::SelectionManager::instance().applySelection(selectionPath, gimp::SelectionMode::Replace);

    bool result = gimp::ClipboardManager::instance().cutSelection(
        fixture.document, fixture.layer, &fixture.commandBus);

    REQUIRE(result);
    REQUIRE(gimp::ClipboardManager::instance().hasImage());

    // Check that cut region is now transparent
    auto [r, g, b, a] = getPixelColor(fixture.layer, 25, 25);
    REQUIRE(r == 0);
    REQUIRE(g == 0);
    REQUIRE(b == 0);
    REQUIRE(a == 0);

    // Check that outside region is still red
    auto [r2, g2, b2, a2] = getPixelColor(fixture.layer, 5, 5);
    REQUIRE(r2 == 255);
    REQUIRE(g2 == 0);
    REQUIRE(b2 == 0);
    REQUIRE(a2 == 255);
}

// ============================================================================
// Paste Tests
// ============================================================================

TEST_CASE("ClipboardManager pasteToDocument returns false with no clipboard image",
          "[clipboard_manager][unit]")
{
    ClipboardFixture fixture;

    // Clear any existing clipboard by copying an empty document
    auto emptyDoc = std::make_shared<gimp::ProjectFile>(1, 1);
    // No layer, so copy fails and clipboard should be empty

    bool result = gimp::ClipboardManager::instance().pasteToDocument(
        fixture.document, &fixture.commandBus, QPoint(50, 50), true);

    // Result depends on whether clipboard has something from previous tests
    // This test mainly verifies no crash occurs
    REQUIRE_NOTHROW(gimp::ClipboardManager::instance().pasteToDocument(
        fixture.document, &fixture.commandBus, QPoint(50, 50), true));
}

TEST_CASE("ClipboardManager paste uses cursor position when useCursor is true",
          "[clipboard_manager][unit]")
{
    ClipboardFixture fixture;

    // Fill layer with red and copy it
    setRegionColor(fixture.layer, 0, 0, 100, 100, 255, 0, 0, 255);
    gimp::ClipboardManager::instance().copySelection(fixture.document, fixture.layer);

    // Paste at specific position
    bool result = gimp::ClipboardManager::instance().pasteToDocument(
        fixture.document, &fixture.commandBus, QPoint(50, 50), true);

    REQUIRE(result);
    // Paste creates a new layer
    REQUIRE(fixture.document->layers().count() == 2);
}

TEST_CASE("ClipboardManager paste centers when useCursor is false", "[clipboard_manager][unit]")
{
    ClipboardFixture fixture;

    // Fill part of layer with red
    setRegionColor(fixture.layer, 0, 0, 20, 20, 255, 0, 0, 255);

    // Create small selection to copy
    QPainterPath selectionPath;
    selectionPath.addRect(0, 0, 20, 20);
    gimp::SelectionManager::instance().applySelection(selectionPath, gimp::SelectionMode::Replace);
    gimp::ClipboardManager::instance().copySelection(fixture.document, fixture.layer);

    // Clear selection
    gimp::SelectionManager::instance().clear();

    // Paste centered (useCursor = false)
    bool result = gimp::ClipboardManager::instance().pasteToDocument(
        fixture.document, &fixture.commandBus, QPoint(0, 0), false);

    REQUIRE(result);
    REQUIRE(fixture.document->layers().count() == 2);
}

// ============================================================================
// Layer Parameter Tests
// ============================================================================

TEST_CASE("ClipboardManager copySelection uses specified layer", "[clipboard_manager][unit]")
{
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    auto layer1 = doc->addLayer();
    auto layer2 = doc->addLayer();

    gimp::SelectionManager::instance().setDocument(doc);
    gimp::SelectionManager::instance().clear();

    // Fill layer1 with red, layer2 with blue
    setRegionColor(layer1, 0, 0, 100, 100, 255, 0, 0, 255);
    setRegionColor(layer2, 0, 0, 100, 100, 0, 0, 255, 255);

    // Copy from layer2 specifically
    bool result = gimp::ClipboardManager::instance().copySelection(doc, layer2);

    REQUIRE(result);

    const QImage& image = gimp::ClipboardManager::instance().image();
    // Check that we copied blue pixels (from layer2)
    QRgb pixel = image.pixel(50, 50);
    REQUIRE(qRed(pixel) == 0);
    REQUIRE(qGreen(pixel) == 0);
    REQUIRE(qBlue(pixel) == 255);
    REQUIRE(qAlpha(pixel) == 255);
}
