/**
 * @file io_manager.cpp
 * @brief Implementation of IOManager for file I/O and project import/export.
 * @author Aless Tosi
 * @date 2025-12-17
 */

#include "io/io_manager.h"

#include "io/utility.h"

#include <nlohmann/json.hpp>
#include <opencv2/imgproc.hpp>

#include <fstream>
#include <iostream>
#include <stdexcept>

/*! @brief Alias for nlohmann::json for convenience. */
using json = nlohmann::json;

namespace gimp {

ProjectFile IOManager::importProject(const std::string& filePath)
{
    std::ifstream inputFile(filePath);
    if (!inputFile.is_open()) {
        throw std::runtime_error("Failed to open project file: " + filePath);
    }

    json projectJson;
    inputFile >> projectJson;
    inputFile.close();

    const int width = projectJson.at("width").get<int>();
    const int height = projectJson.at("height").get<int>();

    ProjectFile project(width, height);

    // Import layers
    if (projectJson.contains("layers")) {
        for (const auto& layerJson : projectJson.at("layers")) {
            // addLayer() creates a new layer with default settings
            auto layer = project.addLayer();

            // Restore layer properties from JSON
            layer->setName(layerJson.at("name").get<std::string>());
            layer->setVisible(layerJson.at("visible").get<bool>());
            layer->setOpacity(layerJson.at("opacity").get<float>());
            layer->setBlendMode(
                string_to_blend_mode(layerJson.at("blend_mode").get<std::string>()));

            // Restore layer data
            const std::vector<uint8_t> layerData = layerJson.at("data").get<std::vector<uint8_t>>();
            if (layerData.size() == layer->data().size()) {
                layer->data() = layerData;
            } else {
                throw std::runtime_error("Layer data size mismatch during import");
            }
        }
    }

    return project;
}

bool IOManager::exportProject(const ProjectFile& project, const std::string& filePath)
{
    try {
        json projectJson;

        projectJson["width"] = project.width();
        projectJson["height"] = project.height();

        json layersJson = json::array();

        // Export each layer
        for (const auto& layer : project.layers()) {
            json layerJson;
            layerJson["name"] = layer->name();
            layerJson["visible"] = layer->visible();
            layerJson["opacity"] = layer->opacity();
            layerJson["blend_mode"] = blend_mode_to_string(layer->blendMode());
            layerJson["width"] = layer->width();
            layerJson["height"] = layer->height();
            layerJson["data"] = layer->data();

            layersJson.push_back(layerJson);
        }

        projectJson["layers"] = layersJson;

        std::ofstream outputFile(filePath);
        if (!outputFile.is_open()) {
            return false;
        }

        outputFile << projectJson.dump(4);  // Pretty-print with 4-space indent
        outputFile.close();

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error exporting project: " << e.what() << '\n';
        return false;
    }
}

ImageFile IOManager::readImage(const std::string& filePath)
{
    const cv::Mat img = cv::imread(filePath, cv::IMREAD_UNCHANGED);
    return {img, filePath};
}

bool IOManager::writeImage(const cv::Mat& mat, const std::string& filePath)
{
    return cv::imwrite(filePath, mat);
}

void IOManager::toGrayscale(cv::Mat& mat)
{
    cv::Mat result;
    if (mat.channels() == 3) {
        cv::cvtColor(mat, result, cv::COLOR_RGB2GRAY);
        mat = result;
    } else if (mat.channels() == 4) {
        cv::cvtColor(mat, result, cv::COLOR_RGBA2GRAY);
        mat = result;
    }
}

void IOManager::toRgb(cv::Mat& mat)
{
    cv::Mat result;
    if (mat.channels() == 1) {
        cv::cvtColor(mat, result, cv::COLOR_GRAY2RGB);
        mat = result;
    } else if (mat.channels() == 4) {
        cv::cvtColor(mat, result, cv::COLOR_RGBA2RGB);
        mat = result;
    }
}

void IOManager::toRgba(cv::Mat& mat)
{
    cv::Mat result;
    if (mat.channels() == 1) {
        cv::cvtColor(mat, result, cv::COLOR_GRAY2RGBA);
        mat = result;
    } else if (mat.channels() == 3) {
        cv::cvtColor(mat, result, cv::COLOR_RGB2RGBA);
        mat = result;
    }
}

}  // namespace gimp
