/**
 * @file test_compositor.cpp
 * @brief Unit tests for SkiaCompositor.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#include "core/layer.h"
#include "render/skia_compositor.h"

#include <include/core/SkBitmap.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkColor.h>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("SkiaCompositor blends layers correctly", "[render][integration]")
{
    // Setup
    gimp::LayerStack stack;

    // Layer 1: Red background (R=255, A=255)
    auto layer1 = std::make_shared<gimp::Layer>(100, 100);
    layer1->setName("Background");
    auto* pixels1 = reinterpret_cast<uint32_t*>(layer1->data().data());
    for (int i = 0; i < 100 * 100; ++i) {
        pixels1[i] = 0xFF0000FF;
    }
    stack.addLayer(layer1);

    // Layer 2: Blue semi-transparent overlay (B=255, A=255, Opacity=0.5)
    auto layer2 = std::make_shared<gimp::Layer>(100, 100);
    layer2->setName("Overlay");
    layer2->setOpacity(0.5F);
    auto* pixels2 = reinterpret_cast<uint32_t*>(layer2->data().data());
    for (int i = 0; i < 100 * 100; ++i) {
        pixels2[i] = 0xFFFF0000;
    }
    stack.addLayer(layer2);

    // Destination bitmap
    SkBitmap destBitmap;
    destBitmap.allocN32Pixels(100, 100);
    SkCanvas canvas(destBitmap);
    canvas.clear(SK_ColorTRANSPARENT);

    // Act
    gimp::SkiaCompositor compositor;
    compositor.compose(&canvas, stack);

    // Assert: Expect blend of Red and Blue (approx R=127, B=127)
    SkColor pixel = destBitmap.getColor(50, 50);

    // Allow for some rounding error (127 or 128)
    int r = SkColorGetR(pixel);
    int g = SkColorGetG(pixel);
    int b = SkColorGetB(pixel);
    int a = SkColorGetA(pixel);

    REQUIRE((r >= 127 && r <= 128));
    REQUIRE(g == 0);
    REQUIRE((b >= 127 && b <= 128));
    REQUIRE(a == 255);
}
