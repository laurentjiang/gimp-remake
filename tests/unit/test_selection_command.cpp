/**
 * @file test_selection_command.cpp
 * @brief Unit tests for SelectionCommand.
 * @author Laurent Jiang
 * @date 2026-02-09
 */

#include "core/commands/selection_command.h"
#include "core/selection_manager.h"
#include "io/project_file.h"

#include <catch2/catch_test_macros.hpp>

#include <QPainterPath>

// ============================================================================
// Basic Property Tests
// ============================================================================

TEST_CASE("SelectionCommand stores description", "[selection_command][unit]")
{
    gimp::SelectionCommand cmd("Select All");
    REQUIRE(cmd.description() == "Select All");
}

// ============================================================================
// State Capture Tests
// ============================================================================

TEST_CASE("SelectionCommand captures before state", "[selection_command][unit]")
{
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    gimp::SelectionManager::instance().setDocument(doc);
    gimp::SelectionManager::instance().clear();

    // Create initial selection
    QPainterPath initialPath;
    initialPath.addRect(10, 10, 50, 50);
    gimp::SelectionManager::instance().applySelection(initialPath, gimp::SelectionMode::Replace);

    gimp::SelectionCommand cmd("Test");
    cmd.captureBeforeState();

    // Modify selection
    QPainterPath newPath;
    newPath.addRect(0, 0, 100, 100);
    gimp::SelectionManager::instance().applySelection(newPath, gimp::SelectionMode::Replace);
    cmd.captureAfterState();

    // Undo should restore before state
    cmd.undo();
    REQUIRE(gimp::SelectionManager::instance().hasSelection());

    // The restored selection should match the before state
    const auto& restored = gimp::SelectionManager::instance().selectionPath();
    REQUIRE(restored.boundingRect().width() == 50);
    REQUIRE(restored.boundingRect().height() == 50);
}

// ============================================================================
// Undo/Redo Tests
// ============================================================================

TEST_CASE("SelectionCommand undo restores previous selection", "[selection_command][unit]")
{
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    gimp::SelectionManager::instance().setDocument(doc);
    gimp::SelectionManager::instance().clear();

    gimp::SelectionCommand cmd("Clear Selection");
    cmd.captureBeforeState();  // Empty selection

    // Create a selection
    QPainterPath selectionPath;
    selectionPath.addRect(20, 20, 60, 60);
    gimp::SelectionManager::instance().applySelection(selectionPath, gimp::SelectionMode::Replace);
    cmd.captureAfterState();

    REQUIRE(gimp::SelectionManager::instance().hasSelection());

    // Undo - should clear the selection (restore before state which was empty)
    cmd.undo();
    REQUIRE_FALSE(gimp::SelectionManager::instance().hasSelection());
}

TEST_CASE("SelectionCommand apply restores after state", "[selection_command][unit]")
{
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    gimp::SelectionManager::instance().setDocument(doc);
    gimp::SelectionManager::instance().clear();

    gimp::SelectionCommand cmd("Create Selection");
    cmd.captureBeforeState();  // Empty

    // Create selection
    QPainterPath selectionPath;
    selectionPath.addRect(10, 10, 80, 80);
    gimp::SelectionManager::instance().applySelection(selectionPath, gimp::SelectionMode::Replace);
    cmd.captureAfterState();

    // Undo first
    cmd.undo();
    REQUIRE_FALSE(gimp::SelectionManager::instance().hasSelection());

    // Redo (apply) - should restore selection
    cmd.apply();
    REQUIRE(gimp::SelectionManager::instance().hasSelection());
}

TEST_CASE("SelectionCommand handles multiple undo/redo cycles", "[selection_command][unit]")
{
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    gimp::SelectionManager::instance().setDocument(doc);
    gimp::SelectionManager::instance().clear();

    gimp::SelectionCommand cmd("Toggle Selection");
    cmd.captureBeforeState();

    QPainterPath path;
    path.addRect(0, 0, 50, 50);
    gimp::SelectionManager::instance().applySelection(path, gimp::SelectionMode::Replace);
    cmd.captureAfterState();

    // Multiple cycles
    for (int i = 0; i < 3; ++i) {
        cmd.undo();
        REQUIRE_FALSE(gimp::SelectionManager::instance().hasSelection());

        cmd.apply();
        REQUIRE(gimp::SelectionManager::instance().hasSelection());
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE("SelectionCommand handles empty before and after states", "[selection_command][unit]")
{
    auto doc = std::make_shared<gimp::ProjectFile>(100, 100);
    doc->addLayer();
    gimp::SelectionManager::instance().setDocument(doc);
    gimp::SelectionManager::instance().clear();

    gimp::SelectionCommand cmd("Empty to Empty");
    cmd.captureBeforeState();  // Empty
    cmd.captureAfterState();   // Still empty

    REQUIRE_NOTHROW(cmd.undo());
    REQUIRE_NOTHROW(cmd.apply());
}
