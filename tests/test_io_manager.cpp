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

TEST_CASE("IOManager reads and writes image files", "[io]")
{
    gimp::IOManager ioManager;

    const std::string testImagePath = "starry_night.jpg";
    REQUIRE(std::filesystem::exists(testImagePath));

    // Read image
    gimp::ImageFile image = ioManager.readImage(testImagePath);
    REQUIRE(!image.empty());
    REQUIRE(image.width() > 0);
    REQUIRE(image.height() > 0);

    SECTION("Convert and write grayscale image")
    {
        cv::Mat grayMat = image.mat().clone();
        ioManager.toGrayscale(grayMat);
        REQUIRE(ioManager.writeImage(grayMat, "starry_night_gray.jpg"));

        gimp::ImageFile grayImage = ioManager.readImage("starry_night_gray.jpg");
        REQUIRE(!grayImage.empty());
        REQUIRE(grayImage.channels() == 1);
    }

    SECTION("Convert and write RGB image")
    {
        cv::Mat rgbMat = image.mat().clone();
        ioManager.toRgb(rgbMat);
        REQUIRE(ioManager.writeImage(rgbMat, "starry_night_rgb.jpg"));

        gimp::ImageFile rgbImage = ioManager.readImage("starry_night_rgb.jpg");
        REQUIRE(!rgbImage.empty());
        REQUIRE(rgbImage.channels() == 3);
    }

    SECTION("Convert and write RGBA image")
    {
        cv::Mat rgbaMat = image.mat().clone();
        ioManager.toRgba(rgbaMat);
        REQUIRE(ioManager.writeImage(rgbaMat, "starry_night_rgba.jpg"));

        // Note: JPG doesn't support alpha, so RGBA will be read back as RGB
        gimp::ImageFile rgbaImage = ioManager.readImage("starry_night_rgba.jpg");
        REQUIRE(!rgbaImage.empty());
    }
}

TEST_CASE("IOManager exports and imports ProjectFile", "[io]")
{
    gimp::IOManager ioManager;

    // Create a test project
    gimp::ProjectFile project(800, 600);

    auto layer1 = project.add_layer();
    layer1->set_name("Background");
    layer1->set_opacity(1.0f);
    layer1->set_blend_mode(gimp::BlendMode::Normal);

    auto layer2 = project.add_layer();
    layer2->set_name("Foreground");
    layer2->set_opacity(0.75f);
    layer2->set_blend_mode(gimp::BlendMode::Multiply);
    layer2->set_visible(false);

    auto layer3 = project.add_layer();
    layer3->set_name("Overlay");
    layer3->set_opacity(0.5f);
    layer3->set_blend_mode(gimp::BlendMode::Overlay);

    SECTION("Export project to JSON")
    {
        REQUIRE(ioManager.exportProject(project, "test_project_export.json"));
        REQUIRE(std::filesystem::exists("test_project_export.json"));
    }

    SECTION("Export and import project preserves data")
    {
        REQUIRE(ioManager.exportProject(project, "test_project_roundtrip.json"));

        gimp::ProjectFile imported = ioManager.importProject("test_project_roundtrip.json");

        // Check project dimensions
        REQUIRE(imported.width() == 800);
        REQUIRE(imported.height() == 600);

        // Check layer count
        REQUIRE(imported.layers().count() == 3);

        // Check layer 1 properties
        auto importedLayer1 = imported.layers()[0];
        REQUIRE(importedLayer1->name() == "Background");
        REQUIRE(importedLayer1->opacity() == 1.0f);
        REQUIRE(importedLayer1->blend_mode() == gimp::BlendMode::Normal);
        REQUIRE(importedLayer1->visible() == true);

        // Check layer 2 properties
        auto importedLayer2 = imported.layers()[1];
        REQUIRE(importedLayer2->name() == "Foreground");
        REQUIRE(importedLayer2->opacity() == 0.75f);
        REQUIRE(importedLayer2->blend_mode() == gimp::BlendMode::Multiply);
        REQUIRE(importedLayer2->visible() == false);

        // Check layer 3 properties
        auto importedLayer3 = imported.layers()[2];
        REQUIRE(importedLayer3->name() == "Overlay");
        REQUIRE(importedLayer3->opacity() == 0.5f);
        REQUIRE(importedLayer3->blend_mode() == gimp::BlendMode::Overlay);
        REQUIRE(importedLayer3->visible() == true);
    }
}
