/**
 * @file test_tool_state_machine.cpp
 * @brief Unit tests for tool state machine transitions.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#include "core/tool.h"
#include "core/tool_factory.h"
#include "core/tools/move_tool.h"
#include "core/tools/pencil_tool.h"

#include <catch2/catch_test_macros.hpp>

namespace {

/**
 * @brief Test tool that tracks state transitions.
 */
class TestTool : public gimp::Tool {
  public:
    [[nodiscard]] std::string id() const override { return "test"; }
    [[nodiscard]] std::string name() const override { return "Test Tool"; }

    int beginCount = 0;
    int continueCount = 0;
    int endCount = 0;
    int cancelCount = 0;

  protected:
    void beginStroke(const gimp::ToolInputEvent& /*event*/) override { ++beginCount; }
    void continueStroke(const gimp::ToolInputEvent& /*event*/) override { ++continueCount; }
    void endStroke(const gimp::ToolInputEvent& /*event*/) override { ++endCount; }
    void cancelStroke() override { ++cancelCount; }
};

gimp::ToolInputEvent makeEvent(int x, int y)
{
    gimp::ToolInputEvent event;
    event.canvasPos = QPoint(x, y);
    event.screenPos = QPoint(x, y);
    event.buttons = Qt::LeftButton;
    event.modifiers = Qt::NoModifier;
    event.pressure = 1.0F;
    return event;
}

}  // namespace

TEST_CASE("Tool starts in Idle state", "[tool]")
{
    TestTool tool;
    REQUIRE(tool.state() == gimp::ToolState::Idle);
}

TEST_CASE("Tool transitions Idle -> Active on mouse press", "[tool]")
{
    TestTool tool;

    REQUIRE(tool.onMousePress(makeEvent(10, 20)));
    REQUIRE(tool.state() == gimp::ToolState::Active);
    REQUIRE(tool.beginCount == 1);
}

TEST_CASE("Tool ignores mouse press when not Idle", "[tool]")
{
    TestTool tool;

    tool.onMousePress(makeEvent(10, 20));
    REQUIRE(tool.state() == gimp::ToolState::Active);

    // Second press should be ignored
    REQUIRE_FALSE(tool.onMousePress(makeEvent(30, 40)));
    REQUIRE(tool.beginCount == 1);
}

TEST_CASE("Tool calls continueStroke during Active state", "[tool]")
{
    TestTool tool;

    tool.onMousePress(makeEvent(10, 20));
    REQUIRE(tool.onMouseMove(makeEvent(15, 25)));
    REQUIRE(tool.onMouseMove(makeEvent(20, 30)));
    REQUIRE(tool.continueCount == 2);
}

TEST_CASE("Tool ignores mouse move when Idle", "[tool]")
{
    TestTool tool;

    REQUIRE_FALSE(tool.onMouseMove(makeEvent(10, 20)));
    REQUIRE(tool.continueCount == 0);
}

TEST_CASE("Tool transitions Active -> Idle on mouse release", "[tool]")
{
    TestTool tool;

    tool.onMousePress(makeEvent(10, 20));
    tool.onMouseMove(makeEvent(15, 25));
    REQUIRE(tool.onMouseRelease(makeEvent(20, 30)));

    REQUIRE(tool.state() == gimp::ToolState::Idle);
    REQUIRE(tool.endCount == 1);
}

TEST_CASE("Tool ignores mouse release when Idle", "[tool]")
{
    TestTool tool;

    REQUIRE_FALSE(tool.onMouseRelease(makeEvent(10, 20)));
    REQUIRE(tool.endCount == 0);
}

TEST_CASE("Tool reset cancels active stroke", "[tool]")
{
    TestTool tool;

    tool.onMousePress(makeEvent(10, 20));
    tool.onMouseMove(makeEvent(15, 25));
    tool.reset();

    REQUIRE(tool.state() == gimp::ToolState::Idle);
    REQUIRE(tool.cancelCount == 1);
}

TEST_CASE("Tool reset does nothing when Idle", "[tool]")
{
    TestTool tool;

    tool.reset();
    REQUIRE(tool.state() == gimp::ToolState::Idle);
    REQUIRE(tool.cancelCount == 0);
}

TEST_CASE("Full stroke cycle: press -> move -> release", "[tool]")
{
    TestTool tool;

    tool.onMousePress(makeEvent(0, 0));
    tool.onMouseMove(makeEvent(10, 10));
    tool.onMouseMove(makeEvent(20, 20));
    tool.onMouseMove(makeEvent(30, 30));
    tool.onMouseRelease(makeEvent(40, 40));

    REQUIRE(tool.state() == gimp::ToolState::Idle);
    REQUIRE(tool.beginCount == 1);
    REQUIRE(tool.continueCount == 3);
    REQUIRE(tool.endCount == 1);
    REQUIRE(tool.cancelCount == 0);
}

TEST_CASE("PencilTool has correct id and name", "[tool]")
{
    gimp::PencilTool pencil;
    REQUIRE(pencil.id() == "pencil");
    REQUIRE(pencil.name() == "Pencil");
}

TEST_CASE("MoveTool has correct id and name", "[tool]")
{
    gimp::MoveTool move;
    REQUIRE(move.id() == "move");
    REQUIRE(move.name() == "Move");
}

TEST_CASE("MoveTool records movement delta", "[tool]")
{
    gimp::MoveTool move;

    move.onMousePress(makeEvent(10, 20));
    move.onMouseMove(makeEvent(15, 25));
    move.onMouseRelease(makeEvent(50, 70));

    REQUIRE(move.lastDelta().x() == 40);
    REQUIRE(move.lastDelta().y() == 50);
}

TEST_CASE("ToolFactory creates and caches tools", "[tool]")
{
    auto& factory = gimp::ToolFactory::instance();
    factory.clearCache();

    factory.registerTool("pencil", []() { return std::make_unique<gimp::PencilTool>(); });
    factory.registerTool("move", []() { return std::make_unique<gimp::MoveTool>(); });

    gimp::Tool* pencil1 = factory.getTool("pencil");
    gimp::Tool* pencil2 = factory.getTool("pencil");

    REQUIRE(pencil1 != nullptr);
    REQUIRE(pencil1 == pencil2);  // Same cached instance

    gimp::Tool* move = factory.getTool("move");
    REQUIRE(move != nullptr);
    REQUIRE(move != pencil1);  // Different tool
}

TEST_CASE("ToolFactory setActiveTool calls lifecycle methods", "[tool]")
{
    auto& factory = gimp::ToolFactory::instance();
    factory.clearCache();

    factory.registerTool("pencil", []() { return std::make_unique<gimp::PencilTool>(); });
    factory.registerTool("move", []() { return std::make_unique<gimp::MoveTool>(); });

    gimp::Tool* pencil = factory.setActiveTool("pencil");
    REQUIRE(pencil != nullptr);
    REQUIRE(factory.activeTool() == pencil);
    REQUIRE(factory.activeToolId() == "pencil");

    gimp::Tool* move = factory.setActiveTool("move");
    REQUIRE(move != nullptr);
    REQUIRE(factory.activeTool() == move);
    REQUIRE(factory.activeToolId() == "move");
}
