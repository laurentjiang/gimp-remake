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

    auto layer1 = project.addLayer();
    layer1->setName("Background");
    layer1->setOpacity(1.0F);
    layer1->setBlendMode(gimp::BlendMode::Normal);

    auto layer2 = project.addLayer();
    layer2->setName("Foreground");
    layer2->setOpacity(0.75F);
    layer2->setBlendMode(gimp::BlendMode::Multiply);
    layer2->setVisible(false);

    auto layer3 = project.addLayer();
    layer3->setName("Overlay");
    layer3->setOpacity(0.5F);
    layer3->setBlendMode(gimp::BlendMode::Overlay);

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
        REQUIRE(importedLayer1->opacity() == 1.0F);
        REQUIRE(importedLayer1->blendMode() == gimp::BlendMode::Normal);
        REQUIRE(importedLayer1->visible() == true);

        // Check layer 2 properties
        auto importedLayer2 = imported.layers()[1];
        REQUIRE(importedLayer2->name() == "Foreground");
        REQUIRE(importedLayer2->opacity() == 0.75F);
        REQUIRE(importedLayer2->blendMode() == gimp::BlendMode::Multiply);
        REQUIRE(importedLayer2->visible() == false);

        // Check layer 3 properties
        auto importedLayer3 = imported.layers()[2];
        REQUIRE(importedLayer3->name() == "Overlay");
        REQUIRE(importedLayer3->opacity() == 0.5F);
        REQUIRE(importedLayer3->blendMode() == gimp::BlendMode::Overlay);
        REQUIRE(importedLayer3->visible() == true);
    }
}
