/**
 * @file test_history_stack.cpp
 * @brief Unit tests for HistoryStack and SimpleHistoryManager.
 * @author Laurent Jiang
 * @date 2026-01-28
 */

#include "core/command.h"
#include "history/history_stack.h"
#include "history/simple_history_manager.h"

#include <catch2/catch_test_macros.hpp>

namespace {

class TestCommand : public gimp::Command {
  public:
    explicit TestCommand(int& value, int delta) : value_(value), delta_(delta) {}

    void apply() override { value_ += delta_; }
    void undo() override { value_ -= delta_; }

  private:
    int& value_;
    int delta_;
};

}  // namespace

// ============================================================================
// HistoryStack Tests
// ============================================================================

TEST_CASE("HistoryStack initializes empty", "[history_stack][unit]")
{
    gimp::HistoryStack stack;

    REQUIRE_FALSE(stack.can_undo());
    REQUIRE_FALSE(stack.can_redo());
    REQUIRE(stack.undo_size() == 0);
    REQUIRE(stack.redo_size() == 0);
}

TEST_CASE("HistoryStack push adds to undo stack", "[history_stack][unit]")
{
    gimp::HistoryStack stack;
    int value = 0;

    auto cmd = std::make_shared<TestCommand>(value, 10);
    stack.push(cmd);

    REQUIRE(stack.can_undo());
    REQUIRE(stack.undo_size() == 1);
    REQUIRE_FALSE(stack.can_redo());
}

TEST_CASE("HistoryStack undo moves command to redo stack", "[history_stack][unit]")
{
    gimp::HistoryStack stack;
    int value = 0;

    auto cmd = std::make_shared<TestCommand>(value, 10);
    cmd->apply();
    stack.push(cmd);

    REQUIRE(value == 10);

    REQUIRE(stack.undo());

    REQUIRE(value == 0);
    REQUIRE_FALSE(stack.can_undo());
    REQUIRE(stack.can_redo());
    REQUIRE(stack.redo_size() == 1);
}

TEST_CASE("HistoryStack redo moves command back to undo stack", "[history_stack][unit]")
{
    gimp::HistoryStack stack;
    int value = 0;

    auto cmd = std::make_shared<TestCommand>(value, 10);
    cmd->apply();
    stack.push(cmd);
    stack.undo();

    REQUIRE(value == 0);

    REQUIRE(stack.redo());

    REQUIRE(value == 10);
    REQUIRE(stack.can_undo());
    REQUIRE_FALSE(stack.can_redo());
}

TEST_CASE("HistoryStack push clears redo stack", "[history_stack][unit]")
{
    gimp::HistoryStack stack;
    int value = 0;

    auto cmd1 = std::make_shared<TestCommand>(value, 10);
    cmd1->apply();
    stack.push(cmd1);
    stack.undo();

    REQUIRE(stack.can_redo());

    auto cmd2 = std::make_shared<TestCommand>(value, 5);
    cmd2->apply();
    stack.push(cmd2);

    REQUIRE_FALSE(stack.can_redo());
    REQUIRE(stack.redo_size() == 0);
}

TEST_CASE("HistoryStack undo on empty stack returns false", "[history_stack][unit]")
{
    gimp::HistoryStack stack;

    REQUIRE_FALSE(stack.undo());
}

TEST_CASE("HistoryStack redo on empty stack returns false", "[history_stack][unit]")
{
    gimp::HistoryStack stack;

    REQUIRE_FALSE(stack.redo());
}

TEST_CASE("HistoryStack clear removes all history", "[history_stack][unit]")
{
    gimp::HistoryStack stack;
    int value = 0;

    auto cmd1 = std::make_shared<TestCommand>(value, 10);
    auto cmd2 = std::make_shared<TestCommand>(value, 20);
    cmd1->apply();
    stack.push(cmd1);
    cmd2->apply();
    stack.push(cmd2);
    stack.undo();

    REQUIRE(stack.can_undo());
    REQUIRE(stack.can_redo());

    stack.clear();

    REQUIRE_FALSE(stack.can_undo());
    REQUIRE_FALSE(stack.can_redo());
    REQUIRE(stack.undo_size() == 0);
    REQUIRE(stack.redo_size() == 0);
}

TEST_CASE("HistoryStack multiple undo/redo operations", "[history_stack][unit]")
{
    gimp::HistoryStack stack;
    int value = 0;

    auto cmd1 = std::make_shared<TestCommand>(value, 10);
    auto cmd2 = std::make_shared<TestCommand>(value, 20);
    auto cmd3 = std::make_shared<TestCommand>(value, 30);

    cmd1->apply();
    stack.push(cmd1);
    cmd2->apply();
    stack.push(cmd2);
    cmd3->apply();
    stack.push(cmd3);

    REQUIRE(value == 60);
    REQUIRE(stack.undo_size() == 3);

    stack.undo();
    REQUIRE(value == 30);

    stack.undo();
    REQUIRE(value == 10);

    stack.redo();
    REQUIRE(value == 30);

    stack.redo();
    REQUIRE(value == 60);
}

// ============================================================================
// SimpleHistoryManager Tests
// ============================================================================

TEST_CASE("SimpleHistoryManager wraps HistoryStack", "[history_manager][unit]")
{
    gimp::SimpleHistoryManager manager;

    REQUIRE_FALSE(manager.can_undo());
    REQUIRE_FALSE(manager.can_redo());
}

TEST_CASE("SimpleHistoryManager push and undo", "[history_manager][unit]")
{
    gimp::SimpleHistoryManager manager;
    int value = 0;

    auto cmd = std::make_shared<TestCommand>(value, 10);
    cmd->apply();
    manager.push(cmd);

    REQUIRE(value == 10);
    REQUIRE(manager.can_undo());

    manager.undo();

    REQUIRE(value == 0);
    REQUIRE(manager.can_redo());
}

TEST_CASE("SimpleHistoryManager with shared HistoryStack", "[history_manager][unit]")
{
    auto stack = std::make_shared<gimp::HistoryStack>();
    gimp::SimpleHistoryManager manager(stack);
    int value = 0;

    auto cmd = std::make_shared<TestCommand>(value, 10);
    cmd->apply();
    manager.push(cmd);

    REQUIRE(stack->undo_size() == 1);
}

TEST_CASE("SimpleHistoryManager clear", "[history_manager][unit]")
{
    gimp::SimpleHistoryManager manager;
    int value = 0;

    auto cmd = std::make_shared<TestCommand>(value, 10);
    cmd->apply();
    manager.push(cmd);

    REQUIRE(manager.undo_size() == 1);

    manager.clear();

    REQUIRE(manager.undo_size() == 0);
    REQUIRE_FALSE(manager.can_undo());
}

