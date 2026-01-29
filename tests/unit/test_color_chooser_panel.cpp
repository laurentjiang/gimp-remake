/**
 * @file test_color_chooser_panel.cpp
 * @brief Unit tests for ColorChooserPanel widget.
 * @author Laurent Jiang
 * @date 2026-01-29
 */

#include "core/event_bus.h"
#include "core/events.h"
#include "ui/color_chooser_panel.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("ColorChooserPanel HSV to RGB conversion", "[ui][unit]")
{
    int r = 0;
    int g = 0;
    int b = 0;

    SECTION("Pure red")
    {
        gimp::ColorChooserPanel::hsvToRgb(0, 255, 255, r, g, b);
        REQUIRE(r == 255);
        REQUIRE(g == 0);
        REQUIRE(b == 0);
    }

    SECTION("Pure green")
    {
        gimp::ColorChooserPanel::hsvToRgb(120, 255, 255, r, g, b);
        REQUIRE(r == 0);
        REQUIRE(g == 255);
        REQUIRE(b == 0);
    }

    SECTION("Pure blue")
    {
        gimp::ColorChooserPanel::hsvToRgb(240, 255, 255, r, g, b);
        REQUIRE(r == 0);
        REQUIRE(g == 0);
        REQUIRE(b == 255);
    }

    SECTION("White (no saturation, max value)")
    {
        gimp::ColorChooserPanel::hsvToRgb(0, 0, 255, r, g, b);
        REQUIRE(r == 255);
        REQUIRE(g == 255);
        REQUIRE(b == 255);
    }

    SECTION("Black (no value)")
    {
        gimp::ColorChooserPanel::hsvToRgb(0, 255, 0, r, g, b);
        REQUIRE(r == 0);
        REQUIRE(g == 0);
        REQUIRE(b == 0);
    }

    SECTION("Gray (no saturation, half value)")
    {
        gimp::ColorChooserPanel::hsvToRgb(0, 0, 128, r, g, b);
        REQUIRE(r == 128);
        REQUIRE(g == 128);
        REQUIRE(b == 128);
    }
}

TEST_CASE("ColorChooserPanel RGB to HSV conversion", "[ui][unit]")
{
    int h = 0;
    int s = 0;
    int v = 0;

    SECTION("Pure red")
    {
        gimp::ColorChooserPanel::rgbToHsv(255, 0, 0, h, s, v);
        REQUIRE(h == 0);
        REQUIRE(s == 255);
        REQUIRE(v == 255);
    }

    SECTION("Pure green")
    {
        gimp::ColorChooserPanel::rgbToHsv(0, 255, 0, h, s, v);
        REQUIRE(h == 120);
        REQUIRE(s == 255);
        REQUIRE(v == 255);
    }

    SECTION("Pure blue")
    {
        gimp::ColorChooserPanel::rgbToHsv(0, 0, 255, h, s, v);
        REQUIRE(h == 240);
        REQUIRE(s == 255);
        REQUIRE(v == 255);
    }

    SECTION("White")
    {
        gimp::ColorChooserPanel::rgbToHsv(255, 255, 255, h, s, v);
        REQUIRE(s == 0);
        REQUIRE(v == 255);
    }

    SECTION("Black")
    {
        gimp::ColorChooserPanel::rgbToHsv(0, 0, 0, h, s, v);
        REQUIRE(s == 0);
        REQUIRE(v == 0);
    }
}

TEST_CASE("ColorChooserPanel HSV-RGB roundtrip", "[ui][unit]")
{
    SECTION("Various colors roundtrip correctly")
    {
        // Test a variety of colors
        std::vector<std::tuple<int, int, int>> testColors = {
            {255, 0,   0  }, // Red
            {0,   255, 0  }, // Green
            {0,   0,   255}, // Blue
            {255, 255, 0  }, // Yellow
            {255, 0,   255}, // Magenta
            {0,   255, 255}, // Cyan
            {128, 64,  32 }, // Brown-ish
            {200, 100, 50 }, // Orange-ish
        };

        for (const auto& [origR, origG, origB] : testColors) {
            int h = 0;
            int s = 0;
            int v = 0;
            gimp::ColorChooserPanel::rgbToHsv(origR, origG, origB, h, s, v);

            int finalR = 0;
            int finalG = 0;
            int finalB = 0;
            gimp::ColorChooserPanel::hsvToRgb(h, s, v, finalR, finalG, finalB);

            // Allow small rounding errors (±1)
            REQUIRE(std::abs(finalR - origR) <= 1);
            REQUIRE(std::abs(finalG - origG) <= 1);
            REQUIRE(std::abs(finalB - origB) <= 1);
        }
    }
}
