/**
 * @file test_shortcut_manager.cpp
 * @brief Unit tests for ShortcutManager and related keyboard shortcut functionality.
 * @author Laurent Jiang
 * @date 2026-01-30
 */

#include "core/tool.h"
#include "core/tool_registry.h"
#include "core/tools/eraser_tool.h"
#include "core/tools/pencil_tool.h"

#include <catch2/catch_test_macros.hpp>

namespace {

/**
 * @brief A minimal tool implementation for testing base class behavior.
 */
class TestTool : public gimp::Tool {
  public:
    [[nodiscard]] std::string id() const override { return "test_tool"; }
    [[nodiscard]] std::string name() const override { return "Test Tool"; }

  protected:
    void beginStroke(const gimp::ToolInputEvent& /*event*/) override {}
    void continueStroke(const gimp::ToolInputEvent& /*event*/) override {}
    void endStroke(const gimp::ToolInputEvent& /*event*/) override {}
};

}  // namespace

// ============================================================================
// Base Tool Virtual Methods Tests
// ============================================================================

TEST_CASE("Base Tool brushSize returns 0 by default", "[shortcut][tool][unit]")
{
    TestTool tool;
    REQUIRE(tool.brushSize() == 0);
}

TEST_CASE("Base Tool setBrushSize does nothing by default", "[shortcut][tool][unit]")
{
    TestTool tool;
    tool.setBrushSize(50);
    REQUIRE(tool.brushSize() == 0);
}

TEST_CASE("PencilTool overrides brushSize correctly", "[shortcut][tool][unit]")
{
    gimp::PencilTool tool;
    REQUIRE(tool.brushSize() == 3);  // Default

    tool.setBrushSize(15);
    REQUIRE(tool.brushSize() == 15);
}

TEST_CASE("EraserTool overrides brushSize correctly", "[shortcut][tool][unit]")
{
    gimp::EraserTool tool;
    REQUIRE(tool.brushSize() == 10);  // Default

    tool.setBrushSize(25);
    REQUIRE(tool.brushSize() == 25);
}

// ============================================================================
// ToolRegistry Shortcut Tests
// ============================================================================

TEST_CASE("ToolRegistry has shortcuts defined for paint tools", "[shortcut][registry][unit]")
{
    const auto& registry = gimp::ToolRegistry::instance();

    const auto* pencil = registry.getTool("pencil");
    REQUIRE(pencil != nullptr);
    REQUIRE(pencil->shortcut == "N");

    const auto* paintbrush = registry.getTool("paintbrush");
    REQUIRE(paintbrush != nullptr);
    REQUIRE(paintbrush->shortcut == "P");

    const auto* eraser = registry.getTool("eraser");
    REQUIRE(eraser != nullptr);
    REQUIRE(eraser->shortcut == "Shift+E");
}

TEST_CASE("ToolRegistry has shortcuts defined for selection tools", "[shortcut][registry][unit]")
{
    const auto& registry = gimp::ToolRegistry::instance();

    const auto* rectSelect = registry.getTool("select_rect");
    REQUIRE(rectSelect != nullptr);
    REQUIRE(rectSelect->shortcut == "R");

    const auto* ellipseSelect = registry.getTool("select_ellipse");
    REQUIRE(ellipseSelect != nullptr);
    REQUIRE(ellipseSelect->shortcut == "E");

    const auto* freeSelect = registry.getTool("select_free");
    REQUIRE(freeSelect != nullptr);
    REQUIRE(freeSelect->shortcut == "F");
}

TEST_CASE("ToolRegistry getAllTools returns tools with shortcuts", "[shortcut][registry][unit]")
{
    const auto& registry = gimp::ToolRegistry::instance();
    auto tools = registry.getAllTools();

    int toolsWithShortcuts = 0;
    for (const auto& tool : tools) {
        if (!tool.shortcut.empty()) {
            ++toolsWithShortcuts;
        }
    }

    REQUIRE(toolsWithShortcuts >= 10);
}
