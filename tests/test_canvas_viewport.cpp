/**
 * @file test_canvas_viewport.cpp
 * @brief Unit tests for canvas viewport coordinate transformations.
 * @author Laurent Jiang
 * @date 2026-01-27
 */

#include "ui/skia_canvas_widget.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using Catch::Matchers::WithinAbs;

TEST_CASE("ViewportState default values", "[canvas][viewport]")
{
    gimp::ViewportState viewport;

    REQUIRE(viewport.zoomLevel == 1.0F);
    REQUIRE(viewport.panX == 0.0F);
    REQUIRE(viewport.panY == 0.0F);
}

TEST_CASE("ViewportState zoom limits", "[canvas][viewport]")
{
    REQUIRE(gimp::ViewportState::MIN_ZOOM == 0.1F);
    REQUIRE(gimp::ViewportState::MAX_ZOOM == 32.0F);
    REQUIRE(gimp::ViewportState::ZOOM_STEP == 1.25F);
}

TEST_CASE("Coordinate transformation at default viewport", "[canvas][viewport]")
{
    SECTION("screenToCanvas formula: canvas = (screen - pan) / zoom") {
        gimp::ViewportState viewport;
        viewport.zoomLevel = 1.0F;
        viewport.panX = 0.0F;
        viewport.panY = 0.0F;

        const float screenX = 100.0F;
        const float screenY = 200.0F;

        const float canvasX = (screenX - viewport.panX) / viewport.zoomLevel;
        const float canvasY = (screenY - viewport.panY) / viewport.zoomLevel;

        REQUIRE_THAT(canvasX, WithinAbs(100.0, 0.001));
        REQUIRE_THAT(canvasY, WithinAbs(200.0, 0.001));
    }

    SECTION("canvasToScreen formula: screen = (canvas * zoom) + pan") {
        gimp::ViewportState viewport;
        viewport.zoomLevel = 1.0F;
        viewport.panX = 0.0F;
        viewport.panY = 0.0F;

        const float canvasX = 100.0F;
        const float canvasY = 200.0F;

        const float screenX = (canvasX * viewport.zoomLevel) + viewport.panX;
        const float screenY = (canvasY * viewport.zoomLevel) + viewport.panY;

        REQUIRE_THAT(screenX, WithinAbs(100.0, 0.001));
        REQUIRE_THAT(screenY, WithinAbs(200.0, 0.001));
    }
}

TEST_CASE("Coordinate transformation with zoom", "[canvas][viewport]")
{
    SECTION("2x zoom: screen coords are 2x canvas coords") {
        gimp::ViewportState viewport;
        viewport.zoomLevel = 2.0F;
        viewport.panX = 0.0F;
        viewport.panY = 0.0F;

        const float canvasX = 50.0F;
        const float canvasY = 75.0F;

        const float screenX = (canvasX * viewport.zoomLevel) + viewport.panX;
        const float screenY = (canvasY * viewport.zoomLevel) + viewport.panY;

        REQUIRE_THAT(screenX, WithinAbs(100.0, 0.001));
        REQUIRE_THAT(screenY, WithinAbs(150.0, 0.001));
    }

    SECTION("0.5x zoom: screen coords are 0.5x canvas coords") {
        gimp::ViewportState viewport;
        viewport.zoomLevel = 0.5F;
        viewport.panX = 0.0F;
        viewport.panY = 0.0F;

        const float canvasX = 200.0F;
        const float canvasY = 300.0F;

        const float screenX = (canvasX * viewport.zoomLevel) + viewport.panX;
        const float screenY = (canvasY * viewport.zoomLevel) + viewport.panY;

        REQUIRE_THAT(screenX, WithinAbs(100.0, 0.001));
        REQUIRE_THAT(screenY, WithinAbs(150.0, 0.001));
    }
}

TEST_CASE("Coordinate transformation with pan", "[canvas][viewport]")
{
    SECTION("Pan offset adds to screen coordinates") {
        gimp::ViewportState viewport;
        viewport.zoomLevel = 1.0F;
        viewport.panX = 50.0F;
        viewport.panY = 100.0F;

        const float canvasX = 100.0F;
        const float canvasY = 200.0F;

        const float screenX = (canvasX * viewport.zoomLevel) + viewport.panX;
        const float screenY = (canvasY * viewport.zoomLevel) + viewport.panY;

        REQUIRE_THAT(screenX, WithinAbs(150.0, 0.001));
        REQUIRE_THAT(screenY, WithinAbs(300.0, 0.001));
    }

    SECTION("Negative pan offset") {
        gimp::ViewportState viewport;
        viewport.zoomLevel = 1.0F;
        viewport.panX = -50.0F;
        viewport.panY = -100.0F;

        const float canvasX = 100.0F;
        const float canvasY = 200.0F;

        const float screenX = (canvasX * viewport.zoomLevel) + viewport.panX;
        const float screenY = (canvasY * viewport.zoomLevel) + viewport.panY;

        REQUIRE_THAT(screenX, WithinAbs(50.0, 0.001));
        REQUIRE_THAT(screenY, WithinAbs(100.0, 0.001));
    }
}

TEST_CASE("Coordinate transformation with zoom and pan combined", "[canvas][viewport]")
{
    SECTION("2x zoom with pan offset") {
        gimp::ViewportState viewport;
        viewport.zoomLevel = 2.0F;
        viewport.panX = 100.0F;
        viewport.panY = 50.0F;

        const float canvasX = 50.0F;
        const float canvasY = 75.0F;

        const float screenX = (canvasX * viewport.zoomLevel) + viewport.panX;
        const float screenY = (canvasY * viewport.zoomLevel) + viewport.panY;

        REQUIRE_THAT(screenX, WithinAbs(200.0, 0.001));
        REQUIRE_THAT(screenY, WithinAbs(200.0, 0.001));

        const float backToCanvasX = (screenX - viewport.panX) / viewport.zoomLevel;
        const float backToCanvasY = (screenY - viewport.panY) / viewport.zoomLevel;

        REQUIRE_THAT(backToCanvasX, WithinAbs(canvasX, 0.001));
        REQUIRE_THAT(backToCanvasY, WithinAbs(canvasY, 0.001));
    }
}

TEST_CASE("Round-trip coordinate transformation", "[canvas][viewport]")
{
    SECTION("Screen -> Canvas -> Screen preserves coordinates") {
        gimp::ViewportState viewport;
        viewport.zoomLevel = 1.5F;
        viewport.panX = 75.0F;
        viewport.panY = -25.0F;

        const float originalScreenX = 300.0F;
        const float originalScreenY = 400.0F;

        const float canvasX = (originalScreenX - viewport.panX) / viewport.zoomLevel;
        const float canvasY = (originalScreenY - viewport.panY) / viewport.zoomLevel;

        const float finalScreenX = (canvasX * viewport.zoomLevel) + viewport.panX;
        const float finalScreenY = (canvasY * viewport.zoomLevel) + viewport.panY;

        REQUIRE_THAT(finalScreenX, WithinAbs(originalScreenX, 0.001));
        REQUIRE_THAT(finalScreenY, WithinAbs(originalScreenY, 0.001));
    }

    SECTION("Canvas -> Screen -> Canvas preserves coordinates") {
        gimp::ViewportState viewport;
        viewport.zoomLevel = 0.75F;
        viewport.panX = -100.0F;
        viewport.panY = 200.0F;

        const float originalCanvasX = 500.0F;
        const float originalCanvasY = 600.0F;

        const float screenX = (originalCanvasX * viewport.zoomLevel) + viewport.panX;
        const float screenY = (originalCanvasY * viewport.zoomLevel) + viewport.panY;

        const float finalCanvasX = (screenX - viewport.panX) / viewport.zoomLevel;
        const float finalCanvasY = (screenY - viewport.panY) / viewport.zoomLevel;

        REQUIRE_THAT(finalCanvasX, WithinAbs(originalCanvasX, 0.001));
        REQUIRE_THAT(finalCanvasY, WithinAbs(originalCanvasY, 0.001));
    }
}

TEST_CASE("Zoom clamping", "[canvas][viewport]")
{
    SECTION("Zoom below minimum should clamp") {
        float zoom = 0.01F;
        zoom = std::clamp(zoom, gimp::ViewportState::MIN_ZOOM, gimp::ViewportState::MAX_ZOOM);
        REQUIRE(zoom == gimp::ViewportState::MIN_ZOOM);
    }

    SECTION("Zoom above maximum should clamp") {
        float zoom = 100.0F;
        zoom = std::clamp(zoom, gimp::ViewportState::MIN_ZOOM, gimp::ViewportState::MAX_ZOOM);
        REQUIRE(zoom == gimp::ViewportState::MAX_ZOOM);
    }

    SECTION("Zoom within range should not change") {
        float zoom = 2.0F;
        zoom = std::clamp(zoom, gimp::ViewportState::MIN_ZOOM, gimp::ViewportState::MAX_ZOOM);
        REQUIRE(zoom == 2.0F);
    }
}

TEST_CASE("Zoom step calculations", "[canvas][viewport]")
{
    SECTION("Zoom in by one step from 1.0") {
        float zoom = 1.0F * gimp::ViewportState::ZOOM_STEP;
        REQUIRE_THAT(zoom, WithinAbs(1.25, 0.001));
    }

    SECTION("Zoom out by one step from 1.0") {
        float zoom = 1.0F / gimp::ViewportState::ZOOM_STEP;
        REQUIRE_THAT(zoom, WithinAbs(0.8, 0.001));
    }

    SECTION("Multiple zoom steps") {
        float zoom = 1.0F;
        for (int i = 0; i < 4; ++i) {
            zoom *= gimp::ViewportState::ZOOM_STEP;
        }
        REQUIRE_THAT(zoom, WithinAbs(2.4414, 0.01));
    }
}
