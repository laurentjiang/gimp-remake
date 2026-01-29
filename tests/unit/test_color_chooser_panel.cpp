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

TEST_CASE("ColorChooserPanel color packing", "[ui][unit]")
{
    SECTION("Pack pure red with full alpha")
    {
        const std::uint32_t color = gimp::ColorChooserPanel::packColor(255, 0, 0, 255);
        REQUIRE(color == 0xFF0000FF);
    }

    SECTION("Pack pure green with full alpha")
    {
        const std::uint32_t color = gimp::ColorChooserPanel::packColor(0, 255, 0, 255);
        REQUIRE(color == 0x00FF00FF);
    }

    SECTION("Pack pure blue with full alpha")
    {
        const std::uint32_t color = gimp::ColorChooserPanel::packColor(0, 0, 255, 255);
        REQUIRE(color == 0x0000FFFF);
    }

    SECTION("Pack white with full alpha")
    {
        const std::uint32_t color = gimp::ColorChooserPanel::packColor(255, 255, 255, 255);
        REQUIRE(color == 0xFFFFFFFF);
    }

    SECTION("Pack black with full alpha")
    {
        const std::uint32_t color = gimp::ColorChooserPanel::packColor(0, 0, 0, 255);
        REQUIRE(color == 0x000000FF);
    }

    SECTION("Pack with partial alpha")
    {
        const std::uint32_t color = gimp::ColorChooserPanel::packColor(128, 64, 32, 128);
        REQUIRE(color == 0x80402080);
    }

    SECTION("Pack with zero alpha")
    {
        const std::uint32_t color = gimp::ColorChooserPanel::packColor(255, 255, 255, 0);
        REQUIRE(color == 0xFFFFFF00);
    }

    SECTION("Default alpha is 255")
    {
        const std::uint32_t color = gimp::ColorChooserPanel::packColor(100, 150, 200);
        REQUIRE(color == 0x6496C8FF);
    }
}

TEST_CASE("ColorChooserPanel color unpacking", "[ui][unit]")
{
    int r = 0;
    int g = 0;
    int b = 0;
    int a = 0;

    SECTION("Unpack pure red with full alpha")
    {
        gimp::ColorChooserPanel::unpackColor(0xFF0000FF, r, g, b, a);
        REQUIRE(r == 255);
        REQUIRE(g == 0);
        REQUIRE(b == 0);
        REQUIRE(a == 255);
    }

    SECTION("Unpack pure green with full alpha")
    {
        gimp::ColorChooserPanel::unpackColor(0x00FF00FF, r, g, b, a);
        REQUIRE(r == 0);
        REQUIRE(g == 255);
        REQUIRE(b == 0);
        REQUIRE(a == 255);
    }

    SECTION("Unpack pure blue with full alpha")
    {
        gimp::ColorChooserPanel::unpackColor(0x0000FFFF, r, g, b, a);
        REQUIRE(r == 0);
        REQUIRE(g == 0);
        REQUIRE(b == 255);
        REQUIRE(a == 255);
    }

    SECTION("Unpack with partial alpha")
    {
        gimp::ColorChooserPanel::unpackColor(0x80402080, r, g, b, a);
        REQUIRE(r == 128);
        REQUIRE(g == 64);
        REQUIRE(b == 32);
        REQUIRE(a == 128);
    }

    SECTION("Unpack with zero alpha")
    {
        gimp::ColorChooserPanel::unpackColor(0xFFFFFF00, r, g, b, a);
        REQUIRE(r == 255);
        REQUIRE(g == 255);
        REQUIRE(b == 255);
        REQUIRE(a == 0);
    }
}

TEST_CASE("ColorChooserPanel pack/unpack roundtrip", "[ui][unit]")
{
    SECTION("Various colors roundtrip correctly")
    {
        std::vector<std::tuple<int, int, int, int>> testColors = {
            {255, 0,   0,   255}, // Red opaque
            {0,   255, 0,   255}, // Green opaque
            {0,   0,   255, 255}, // Blue opaque
            {128, 64,  32,  128}, // Partial alpha
            {100, 150, 200, 50 }, // Low alpha
            {0,   0,   0,   0  }, // Fully transparent black
            {255, 255, 255, 0  }, // Fully transparent white
        };

        for (const auto& [origR, origG, origB, origA] : testColors) {
            const std::uint32_t packed = gimp::ColorChooserPanel::packColor(origR, origG, origB, origA);

            int finalR = 0;
            int finalG = 0;
            int finalB = 0;
            int finalA = 0;
            gimp::ColorChooserPanel::unpackColor(packed, finalR, finalG, finalB, finalA);

            REQUIRE(finalR == origR);
            REQUIRE(finalG == origG);
            REQUIRE(finalB == origB);
            REQUIRE(finalA == origA);
        }
    }
}

TEST_CASE("ColorChooserPanel hex color parsing", "[ui][unit]")
{
    int r = 0;
    int g = 0;
    int b = 0;

    SECTION("Parse valid hex with hash")
    {
        REQUIRE(gimp::ColorChooserPanel::parseHexColor("#FF0000", r, g, b));
        REQUIRE(r == 255);
        REQUIRE(g == 0);
        REQUIRE(b == 0);
    }

    SECTION("Parse valid hex without hash")
    {
        REQUIRE(gimp::ColorChooserPanel::parseHexColor("00FF00", r, g, b));
        REQUIRE(r == 0);
        REQUIRE(g == 255);
        REQUIRE(b == 0);
    }

    SECTION("Parse lowercase hex")
    {
        REQUIRE(gimp::ColorChooserPanel::parseHexColor("#abcdef", r, g, b));
        REQUIRE(r == 0xAB);
        REQUIRE(g == 0xCD);
        REQUIRE(b == 0xEF);
    }

    SECTION("Parse mixed case hex")
    {
        REQUIRE(gimp::ColorChooserPanel::parseHexColor("#AbCdEf", r, g, b));
        REQUIRE(r == 0xAB);
        REQUIRE(g == 0xCD);
        REQUIRE(b == 0xEF);
    }

    SECTION("Reject too short")
    {
        REQUIRE_FALSE(gimp::ColorChooserPanel::parseHexColor("#FFF", r, g, b));
    }

    SECTION("Reject too long")
    {
        REQUIRE_FALSE(gimp::ColorChooserPanel::parseHexColor("#FFFFFFF", r, g, b));
    }

    SECTION("Reject empty string")
    {
        REQUIRE_FALSE(gimp::ColorChooserPanel::parseHexColor("", r, g, b));
    }

    SECTION("Reject invalid characters")
    {
        REQUIRE_FALSE(gimp::ColorChooserPanel::parseHexColor("#GGGGGG", r, g, b));
    }

    SECTION("Reject just hash")
    {
        REQUIRE_FALSE(gimp::ColorChooserPanel::parseHexColor("#", r, g, b));
    }
}
