/**
 * @file test_io_manager.cpp
 * @brief Unit tests for IOManager image read/write, color conversion, and project import/export.
 * @author Aless Tosi
 * @date 2025-12-17
 */

#include "io/io_manager.h"
#include "io/project_file.h"

#include <filesystem>

#include <catch2/catch_test_macros.hpp>

using namespace gimp;

TEST_CASE("IOManager reads and writes image files", "[io]")
{
    IOManager io_manager;

    const std::string test_image_path = "starry_night.jpg";
    REQUIRE(std::filesystem::exists(test_image_path));

    // Read image
    ImageFile image = io_manager.readImage(test_image_path);
    REQUIRE(!image.empty());
    REQUIRE(image.width() > 0);
    REQUIRE(image.height() > 0);

    SECTION("Convert and write grayscale image")
    {
        cv::Mat gray_mat = image.mat().clone();
        io_manager.toGrayscale(gray_mat);
        REQUIRE(io_manager.writeImage(gray_mat, "starry_night_gray.jpg"));

        ImageFile gray_image = io_manager.readImage("starry_night_gray.jpg");
        REQUIRE(!gray_image.empty());
        REQUIRE(gray_image.channels() == 1);
    }

    SECTION("Convert and write RGB image")
    {
        cv::Mat rgb_mat = image.mat().clone();
        io_manager.toRgb(rgb_mat);
        REQUIRE(io_manager.writeImage(rgb_mat, "starry_night_rgb.jpg"));

        ImageFile rgb_image = io_manager.readImage("starry_night_rgb.jpg");
        REQUIRE(!rgb_image.empty());
        REQUIRE(rgb_image.channels() == 3);
    }

    SECTION("Convert and write RGBA image")
    {
        cv::Mat rgba_mat = image.mat().clone();
        io_manager.toRgba(rgba_mat);
        REQUIRE(io_manager.writeImage(rgba_mat, "starry_night_rgba.jpg"));

        // Note: JPG doesn't support alpha, so RGBA will be read back as RGB
        ImageFile rgba_image = io_manager.readImage("starry_night_rgba.jpg");
        REQUIRE(!rgba_image.empty());
    }
}

TEST_CASE("IOManager exports and imports ProjectFile", "[io]")
{
    IOManager io_manager;

    // Create a test project
    ProjectFile project(800, 600);

    auto layer1 = project.add_layer();
    layer1->set_name("Background");
    layer1->set_opacity(1.0f);
    layer1->set_blend_mode(BlendMode::Normal);

    auto layer2 = project.add_layer();
    layer2->set_name("Foreground");
    layer2->set_opacity(0.75f);
    layer2->set_blend_mode(BlendMode::Multiply);
    layer2->set_visible(false);

    auto layer3 = project.add_layer();
    layer3->set_name("Overlay");
    layer3->set_opacity(0.5f);
    layer3->set_blend_mode(BlendMode::Overlay);

    SECTION("Export project to JSON")
    {
        REQUIRE(io_manager.exportProject(project, "test_project_export.json"));
        REQUIRE(std::filesystem::exists("test_project_export.json"));
    }

    SECTION("Export and import project preserves data")
    {
        REQUIRE(io_manager.exportProject(project, "test_project_roundtrip.json"));

        ProjectFile imported = io_manager.importProject("test_project_roundtrip.json");

        // Check project dimensions
        REQUIRE(imported.width() == 800);
        REQUIRE(imported.height() == 600);

        // Check layer count
        REQUIRE(imported.layers().count() == 3);

        // Check layer 1 properties
        auto imported_layer1 = imported.layers()[0];
        REQUIRE(imported_layer1->name() == "Background");
        REQUIRE(imported_layer1->opacity() == 1.0f);
        REQUIRE(imported_layer1->blend_mode() == BlendMode::Normal);
        REQUIRE(imported_layer1->visible() == true);

        // Check layer 2 properties
        auto imported_layer2 = imported.layers()[1];
        REQUIRE(imported_layer2->name() == "Foreground");
        REQUIRE(imported_layer2->opacity() == 0.75f);
        REQUIRE(imported_layer2->blend_mode() == BlendMode::Multiply);
        REQUIRE(imported_layer2->visible() == false);

        // Check layer 3 properties
        auto imported_layer3 = imported.layers()[2];
        REQUIRE(imported_layer3->name() == "Overlay");
        REQUIRE(imported_layer3->opacity() == 0.5f);
        REQUIRE(imported_layer3->blend_mode() == BlendMode::Overlay);
        REQUIRE(imported_layer3->visible() == true);
    }
}
