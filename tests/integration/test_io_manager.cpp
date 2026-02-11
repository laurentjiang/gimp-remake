/**
 * @file test_io_manager.cpp
 * @brief Integration tests for IOManager image read/write, color conversion, and project
 * import/export.
 * @author Aless Tosi
 * @date 2025-12-17
 */

#include "io/binary_project_reader.h"
#include "io/binary_project_writer.h"
#include "io/io_manager.h"
#include "io/project_file.h"

#include <QPainterPath>

#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>

#include <catch2/catch_test_macros.hpp>

namespace {
// Get the source directory from CMake-defined macro or use relative path
#ifdef SOURCE_DIR
const std::filesystem::path SOURCE_ROOT = SOURCE_DIR;
#else
const std::filesystem::path SOURCE_ROOT = ".";
#endif

// Test input/output directories (relative to source root)
const std::string TEST_INPUT_IMAGE = (SOURCE_ROOT / "tests/img/starry_night.jpg").string();
const std::string TEST_OUTPUT_DIR = (SOURCE_ROOT / "tests/img/generated/").string();

bool pathsMatch(const QPainterPath& a, const QPainterPath& b)
{
    if (a.elementCount() != b.elementCount()) {
        return false;
    }

    for (int i = 0; i < a.elementCount(); ++i) {
        const auto ea = a.elementAt(i);
        const auto eb = b.elementAt(i);
        if (ea.type != eb.type) {
            return false;
        }

        if (std::abs(ea.x - eb.x) > 0.001 || std::abs(ea.y - eb.y) > 0.001) {
            return false;
        }
    }

    return true;
}
}  // namespace

TEST_CASE("IOManager reads and writes image files", "[io][integration]")
{
    gimp::IOManager ioManager;

    REQUIRE(std::filesystem::exists(TEST_INPUT_IMAGE));

    // Read image
    gimp::ImageFile image = ioManager.readImage(TEST_INPUT_IMAGE);
    REQUIRE(!image.empty());
    REQUIRE(image.width() > 0);
    REQUIRE(image.height() > 0);

    SECTION("Convert and write grayscale image")
    {
        cv::Mat grayMat = image.mat().clone();
        ioManager.toGrayscale(grayMat);
        const std::string outputPath = TEST_OUTPUT_DIR + "starry_night_gray.jpg";
        REQUIRE(ioManager.writeImage(grayMat, outputPath));

        gimp::ImageFile grayImage = ioManager.readImage(outputPath);
        REQUIRE(!grayImage.empty());
        REQUIRE(grayImage.channels() == 1);
    }

    SECTION("Convert and write RGB image")
    {
        cv::Mat rgbMat = image.mat().clone();
        ioManager.toRgb(rgbMat);
        const std::string outputPath = TEST_OUTPUT_DIR + "starry_night_rgb.jpg";
        REQUIRE(ioManager.writeImage(rgbMat, outputPath));

        gimp::ImageFile rgbImage = ioManager.readImage(outputPath);
        REQUIRE(!rgbImage.empty());
        REQUIRE(rgbImage.channels() == 3);
    }

    SECTION("Convert and write RGBA image")
    {
        cv::Mat rgbaMat = image.mat().clone();
        ioManager.toRgba(rgbaMat);
        const std::string outputPath = TEST_OUTPUT_DIR + "starry_night_rgba.jpg";
        REQUIRE(ioManager.writeImage(rgbaMat, outputPath));

        // Note: JPG doesn't support alpha, so RGBA will be read back as RGB
        gimp::ImageFile rgbaImage = ioManager.readImage(outputPath);
        REQUIRE(!rgbaImage.empty());
    }
}

TEST_CASE("IOManager exports and imports ProjectFile", "[io][integration]")
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

    QPainterPath selectionPath;
    selectionPath.moveTo(10.0, 15.0);
    selectionPath.lineTo(200.0, 25.0);
    selectionPath.cubicTo(220.0, 30.0, 240.0, 50.0, 260.0, 70.0);
    selectionPath.closeSubpath();
    project.setSelectionPath(selectionPath);

    SECTION("Export project to JSON")
    {
        const std::string outputPath = TEST_OUTPUT_DIR + "test_project_export.json";
        REQUIRE(ioManager.exportProject(project, outputPath));
        REQUIRE(std::filesystem::exists(outputPath));
    }

    SECTION("Export and import project preserves data")
    {
        const std::string outputPath = TEST_OUTPUT_DIR + "test_project_roundtrip.json";
        REQUIRE(ioManager.exportProject(project, outputPath));

        gimp::ProjectFile imported = ioManager.importProject(outputPath);

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

        REQUIRE(!imported.selectionPath().isEmpty());
        REQUIRE(pathsMatch(imported.selectionPath(), selectionPath));
    }
}

TEST_CASE("IOManager binary format roundtrip", "[io][integration][binary]")
{
    gimp::IOManager ioManager;

    // Create a test project with actual pixel data
    gimp::ProjectFile project(100, 100);

    auto layer1 = project.addLayer();
    layer1->setName("Red Layer");
    layer1->setOpacity(0.8F);
    layer1->setBlendMode(gimp::BlendMode::Normal);
    // Fill with red color (RGBA)
    auto& data1 = layer1->data();
    for (size_t i = 0; i < data1.size(); i += 4) {
        data1[i] = 255;      // R
        data1[i + 1] = 0;    // G
        data1[i + 2] = 0;    // B
        data1[i + 3] = 255;  // A
    }

    auto layer2 = project.addLayer();
    layer2->setName("Blue Layer");
    layer2->setOpacity(0.5F);
    layer2->setBlendMode(gimp::BlendMode::Multiply);
    layer2->setVisible(false);
    // Fill with blue color
    auto& data2 = layer2->data();
    for (size_t i = 0; i < data2.size(); i += 4) {
        data2[i] = 0;        // R
        data2[i + 1] = 0;    // G
        data2[i + 2] = 255;  // B
        data2[i + 3] = 128;  // A (semi-transparent)
    }

    QPainterPath selectionPath;
    selectionPath.addRect(10, 10, 50, 50);
    project.setSelectionPath(selectionPath);

    SECTION("Save and load binary project preserves all data")
    {
        const std::filesystem::path outputPath =
            std::filesystem::path(TEST_OUTPUT_DIR) / "test_binary_roundtrip.gimp";

        // Save project
        auto writeResult = ioManager.saveProject(project, outputPath);
        REQUIRE(writeResult.IsOk());
        REQUIRE(std::filesystem::exists(outputPath));

        // Load project
        auto readResult = ioManager.loadProject(outputPath);
        REQUIRE(readResult.IsOk());

        auto imported = readResult.Value();
        REQUIRE(imported != nullptr);

        // Check dimensions
        REQUIRE(imported->width() == 100);
        REQUIRE(imported->height() == 100);

        // Check layer count
        REQUIRE(imported->layers().count() == 2);

        // Check layer 1 properties and data
        auto importedLayer1 = imported->layers()[0];
        REQUIRE(importedLayer1->name() == "Red Layer");
        REQUIRE(importedLayer1->opacity() == 0.8F);
        REQUIRE(importedLayer1->blendMode() == gimp::BlendMode::Normal);
        REQUIRE(importedLayer1->visible() == true);

        // Verify pixel data
        const auto& importedData1 = importedLayer1->data();
        REQUIRE(importedData1.size() == data1.size());
        REQUIRE(std::memcmp(importedData1.data(), data1.data(), data1.size()) == 0);

        // Check layer 2 properties and data
        auto importedLayer2 = imported->layers()[1];
        REQUIRE(importedLayer2->name() == "Blue Layer");
        REQUIRE(importedLayer2->opacity() == 0.5F);
        REQUIRE(importedLayer2->blendMode() == gimp::BlendMode::Multiply);
        REQUIRE(importedLayer2->visible() == false);

        const auto& importedData2 = importedLayer2->data();
        REQUIRE(importedData2.size() == data2.size());
        REQUIRE(std::memcmp(importedData2.data(), data2.data(), data2.size()) == 0);

        // Check selection path
        REQUIRE(!imported->selectionPath().isEmpty());
        REQUIRE(pathsMatch(imported->selectionPath(), selectionPath));

        // Check file path is set
        REQUIRE(imported->filePath().has_value());
        REQUIRE(imported->filePath().value() == outputPath);
    }
}

TEST_CASE("Binary format handles large images efficiently",
          "[io][integration][binary][performance]")
{
    gimp::IOManager ioManager;

    // Create a 1920x1080 image (Full HD)
    gimp::ProjectFile project(1920, 1080);

    auto layer = project.addLayer();
    layer->setName("Large Layer");

    // Fill with gradient pattern
    auto& data = layer->data();
    for (int y = 0; y < 1080; ++y) {
        for (int x = 0; x < 1920; ++x) {
            const size_t idx = (static_cast<size_t>(y) * 1920 + static_cast<size_t>(x)) * 4;
            data[idx] = static_cast<uint8_t>(x % 256);
            data[idx + 1] = static_cast<uint8_t>(y % 256);
            data[idx + 2] = static_cast<uint8_t>((x + y) % 256);
            data[idx + 3] = 255;
        }
    }

    const std::filesystem::path outputPath =
        std::filesystem::path(TEST_OUTPUT_DIR) / "test_large_binary.gimp";

    // Time the save operation
    auto saveStart = std::chrono::high_resolution_clock::now();
    auto writeResult = ioManager.saveProject(project, outputPath);
    auto saveEnd = std::chrono::high_resolution_clock::now();

    REQUIRE(writeResult.IsOk());

    auto saveDuration =
        std::chrono::duration_cast<std::chrono::milliseconds>(saveEnd - saveStart).count();

    // Save should complete in under 2 seconds (generous for CI)
    REQUIRE(saveDuration < 2000);

    // Check file size - compressed should be significantly smaller than raw
    // Raw size: 1920 * 1080 * 4 = ~8MB
    auto fileSize = std::filesystem::file_size(outputPath);
    REQUIRE(fileSize < 5 * 1024 * 1024);  // Should be under 5MB compressed

    // Time the load operation
    auto loadStart = std::chrono::high_resolution_clock::now();
    auto readResult = ioManager.loadProject(outputPath);
    auto loadEnd = std::chrono::high_resolution_clock::now();

    REQUIRE(readResult.IsOk());

    auto loadDuration =
        std::chrono::duration_cast<std::chrono::milliseconds>(loadEnd - loadStart).count();

    // Load should complete in under 2 seconds
    REQUIRE(loadDuration < 2000);

    // Verify data integrity
    auto imported = readResult.Value();
    REQUIRE(imported->width() == 1920);
    REQUIRE(imported->height() == 1080);
    REQUIRE(imported->layers().count() == 1);

    const auto& importedData = imported->layers()[0]->data();
    REQUIRE(importedData.size() == data.size());
    REQUIRE(std::memcmp(importedData.data(), data.data(), data.size()) == 0);
}

TEST_CASE("Binary reader handles corrupted files", "[io][integration][binary][error]")
{
    const std::filesystem::path testDir = std::filesystem::path(TEST_OUTPUT_DIR);

    SECTION("Rejects file with invalid magic header")
    {
        const std::filesystem::path corruptPath = testDir / "corrupt_magic.gimp";

        // Write a file with wrong magic bytes
        std::ofstream file(corruptPath, std::ios::binary);
        const char badData[] = "BADM\x01\x00\x00\x00\x64\x00\x00\x00\x64\x00\x00\x00";
        file.write(badData, sizeof(badData) - 1);
        file.close();

        auto result = gimp::BinaryProjectReader::read(corruptPath);
        REQUIRE(result.IsError());
        REQUIRE(result.Error().GetCode() == gimp::error::ErrorCode::IOCorruptedFile);
    }

    SECTION("Rejects file with unsupported version")
    {
        const std::filesystem::path versionPath = testDir / "future_version.gimp";

        // Write a file with version 99
        std::ofstream file(versionPath, std::ios::binary);
        const uint32_t magic = 0x504D4947;  // "GIMP"
        const uint32_t version = 99;        // Unsupported version
        const uint32_t width = 100;
        const uint32_t height = 100;
        file.write(reinterpret_cast<const char*>(&magic), 4);
        file.write(reinterpret_cast<const char*>(&version), 4);
        file.write(reinterpret_cast<const char*>(&width), 4);
        file.write(reinterpret_cast<const char*>(&height), 4);
        file.close();

        auto result = gimp::BinaryProjectReader::read(versionPath);
        REQUIRE(result.IsError());
        REQUIRE(result.Error().GetCode() == gimp::error::ErrorCode::IOUnsupportedFormat);
    }

    SECTION("Returns error for non-existent file")
    {
        gimp::IOManager ioManager;
        auto result = ioManager.loadProject(testDir / "nonexistent_file.gimp");
        REQUIRE(result.IsError());
        REQUIRE(result.Error().GetCode() == gimp::error::ErrorCode::IOFileNotFound);
    }
}

TEST_CASE("IOManager auto-detects file format", "[io][integration][binary]")
{
    gimp::IOManager ioManager;

    // Create a simple project
    gimp::ProjectFile project(50, 50);
    auto layer = project.addLayer();
    layer->setName("Test Layer");

    SECTION("Loads binary format when magic header present")
    {
        const std::filesystem::path binaryPath =
            std::filesystem::path(TEST_OUTPUT_DIR) / "auto_detect_binary.gimp";

        auto writeResult = ioManager.saveProject(project, binaryPath);
        REQUIRE(writeResult.IsOk());

        auto readResult = ioManager.loadProject(binaryPath);
        REQUIRE(readResult.IsOk());
        REQUIRE(readResult.Value()->layers().count() == 1);
        REQUIRE(readResult.Value()->layers()[0]->name() == "Test Layer");
    }

    SECTION("Falls back to JSON for legacy files")
    {
        const std::filesystem::path jsonPath =
            std::filesystem::path(TEST_OUTPUT_DIR) / "auto_detect_json.json";

        // Use legacy export
        REQUIRE(ioManager.exportProject(project, jsonPath.string()));

        auto readResult = ioManager.loadProject(jsonPath);
        REQUIRE(readResult.IsOk());
        REQUIRE(readResult.Value()->layers().count() == 1);
        REQUIRE(readResult.Value()->layers()[0]->name() == "Test Layer");
    }
}
