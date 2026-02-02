/**
 * @file test_gradient_tool.cpp
 * @brief Unit tests for GradientTool.
 * @author Aless Tosi
 * @date 2026-02-01
 */

#include "core/command_bus.h"
#include "core/layer.h"
#include "core/tool_factory.h"
#include "core/tools/gradient_tool.h"

#include <catch2/catch_test_macros.hpp>

namespace gimp {

TEST_CASE("GradientTool has correct id", "[gradient_tool][unit]")
{
    GradientTool tool;
    REQUIRE(tool.id() == "gradient");
}

TEST_CASE("GradientTool has correct name", "[gradient_tool][unit]")
{
    GradientTool tool;
    REQUIRE(tool.name() == "Gradient");
}

TEST_CASE("GradientTool starts in Idle state", "[gradient_tool][unit]")
{
    GradientTool tool;
    REQUIRE(tool.state() == ToolState::Idle);
}

TEST_CASE("GradientTool sets and gets mode", "[gradient_tool][unit]")
{
    GradientTool tool;

    tool.setMode(GradientMode::Linear);
    REQUIRE(tool.mode() == GradientMode::Linear);

    tool.setMode(GradientMode::Radial);
    REQUIRE(tool.mode() == GradientMode::Radial);
}

TEST_CASE("GradientTool sets and gets fill mode", "[gradient_tool][unit]")
{
    GradientTool tool;

    tool.setFill(GradientFill::ForegroundToBackground);
    REQUIRE(tool.fill() == GradientFill::ForegroundToBackground);

    tool.setFill(GradientFill::ForegroundToTransparent);
    REQUIRE(tool.fill() == GradientFill::ForegroundToTransparent);
}

TEST_CASE("GradientTool sets and gets gradient shape", "[gradient_tool][unit]")
{
    GradientTool tool;

    tool.setGradientShape(10, 20, 30, 40);
    REQUIRE(tool.gradientStart().x() == 10);
    REQUIRE(tool.gradientStart().y() == 20);
    REQUIRE(tool.gradientEnd().x() == 30);
    REQUIRE(tool.gradientEnd().y() == 40);
}

TEST_CASE("GradientTool handles degenerate gradient (zero length)", "[gradient_tool][unit]")
{
    GradientTool tool;

    ToolFactory::instance().setForegroundColor(0xFF0000FF);  // Red

    // Set degenerate gradient shape (same start and end)
    tool.setGradientShape(25, 25, 25, 25);

    // Should not crash when accessing shape
    REQUIRE(tool.gradientStart() == tool.gradientEnd());
}

TEST_CASE("GradientTool color interpolation", "[gradient_tool][unit]")
{
    // Test lerp between red and blue
    std::uint32_t red = 0xFF0000FF;
    std::uint32_t blue = 0x0000FFFF;

    // At t=0, should be red
    std::uint32_t color0 = GradientTool::lerpColor(red, blue, 0.0F);
    REQUIRE(color0 == red);

    // At t=1, should be blue
    std::uint32_t color1 = GradientTool::lerpColor(red, blue, 1.0F);
    REQUIRE(color1 == blue);

    // At t=0.5, should be interpolated (purple-ish)
    std::uint32_t colorMid = GradientTool::lerpColor(red, blue, 0.5F);
    // Check that we have red and blue components
    REQUIRE(((colorMid >> 24) & 0xFF) > 0);  // Some red
    REQUIRE(((colorMid >> 8) & 0xFF) > 0);   // Some blue
}

}  // namespace gimp
