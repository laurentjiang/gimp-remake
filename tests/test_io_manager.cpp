/**
 * @file test_io_manager.cpp
 * @brief Unit tests for IOManager image read/write and color conversion.
 * @author Aless Tosi
 * @date 2025-12-17
 */

#include <catch2/catch_test_macros.hpp>
#include "io/io_manager.h"
#include "io/opencv_image.h"
#include <filesystem>

using namespace gimp;

TEST_CASE("IOManager reads and writes images", "[io]") {
    IOManager io_manager;

    // Use the provided starry_night.jpg for testing
    const std::string test_image_path = "starry_night.jpg";
    // Ensure the test image exists
    REQUIRE(std::filesystem::exists(test_image_path));

    // Read image
    std::shared_ptr<Image> image = io_manager.read_image(test_image_path);
    // We check for non-null image
    REQUIRE(image);
    // We check that the image is not empty
    REQUIRE(!image->empty());

    SECTION("Write grayscale") {
        auto img_gray = std::make_shared<OpenCVImage>(image->mat().clone(), image->file_path());
        // We test conversion to Grayscale (1 channel)
        img_gray->to_grayscale();
        // Write Grayscale image
        REQUIRE(io_manager.write_image(*img_gray, "starry_night_gray.jpg"));
        // Read Grayscale image
        auto gray = io_manager.read_image("starry_night_gray.jpg");
        // We check for non-null as some formats may not support grayscale
        REQUIRE(gray);
        // We expect 1 channel for grayscale
        REQUIRE(gray->channels() == 1);
    }

    SECTION("Write RGB") {
        auto img_rgb = std::make_shared<OpenCVImage>(image->mat().clone(), image->file_path());
        // We test conversion to RGB (3 channels)
        img_rgb->to_rgb();
        // Write RGBimage
        REQUIRE(io_manager.write_image(*img_rgb, "starry_night_rgb.jpg"));
        // Read RGB image
        auto rgb = io_manager.read_image("starry_night_rgb.jpg");
        // We check for non-null as some formats may not support RGB
        REQUIRE(rgb);
        // We expect 3 channels for RGB
        REQUIRE(rgb->channels() == 3);
    }

    SECTION("Write RGBA") {
        auto img_rgba = std::make_shared<OpenCVImage>(image->mat().clone(), image->file_path());
        // We test conversion to RGBA (4 channels)
        img_rgba->to_rgba();
        // Write RGBA image
        REQUIRE(io_manager.write_image(*img_rgba, "starry_night_rgba.jpg"));
        // Read RGBA image
        auto rgba = io_manager.read_image("starry_night_rgba.jpg");
        // We check for non-null as some formats may not support RGBA
        REQUIRE(rgba);
        // We expect 4 channels for RGBA
        REQUIRE(rgba->channels() == 4);
    }
}
