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

using namespace gimp;

TEST_CASE("SkiaCompositor blends layers correctly", "[render]")
{
    // Setup
    LayerStack stack;

    // Layer 1: Red background (R=255, A=255)
    auto layer1 = std::make_shared<Layer>(100, 100);
    layer1->set_name("Background");
    uint32_t* pixels1 = (uint32_t*)layer1->data().data();
    for (int i = 0; i < 100 * 100; ++i) {
        pixels1[i] = 0xFF0000FF;
    }
    stack.add_layer(layer1);

    // Layer 2: Blue semi-transparent overlay (B=255, A=255, Opacity=0.5)
    auto layer2 = std::make_shared<Layer>(100, 100);
    layer2->set_name("Overlay");
    layer2->set_opacity(0.5f);
    uint32_t* pixels2 = (uint32_t*)layer2->data().data();
    for (int i = 0; i < 100 * 100; ++i) {
        pixels2[i] = 0xFFFF0000;
    }
    stack.add_layer(layer2);

    // Destination bitmap
    SkBitmap destBitmap;
    destBitmap.allocN32Pixels(100, 100);
    SkCanvas canvas(destBitmap);
    canvas.clear(SK_ColorTRANSPARENT);

    // Act
    SkiaCompositor compositor;
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
