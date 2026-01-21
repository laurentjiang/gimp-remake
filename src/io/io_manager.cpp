/**
 * @file io_manager.cpp
 * @brief Implementation of IOManager for file I/O and project import/export.
 * @author Aless Tosi
 * @date 2025-12-17
 */

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <opencv2/imgproc.hpp>
#include <stdexcept>

#include "io/io_manager.h"
#include "io/utility.h"

using json = nlohmann::json;

namespace gimp {

ProjectFile IOManager::importProject(const std::string& file_path) {
    std::ifstream input_file(file_path);
    if (!input_file.is_open()) {
        throw std::runtime_error("Failed to open project file: " + file_path);
    }

    json project_json;
    input_file >> project_json;
    input_file.close();

    int width = project_json.at("width").get<int>();
    int height = project_json.at("height").get<int>();

    ProjectFile project(width, height);

    // Import layers
    if (project_json.contains("layers")) {
        for (const auto& layer_json : project_json.at("layers")) {
            auto layer = std::make_shared<Layer>(width, height);

            // Restore layer properties
            layer->set_name(layer_json.at("name").get<std::string>());
            layer->set_visible(layer_json.at("visible").get<bool>());
            layer->set_opacity(layer_json.at("opacity").get<float>());
            layer->set_blend_mode(string_to_blend_mode(layer_json.at("blend_mode").get<std::string>()));

            // Restore layer data
            std::vector<uint8_t> layer_data = layer_json.at("data").get<std::vector<uint8_t>>();
            if (layer_data.size() == layer->data().size()) {
                layer->data() = layer_data;
            } else {
                throw std::runtime_error("Layer data size mismatch during import");
            }

            project.add_layer();  // This adds to the layer stack
        }
    }

    return project;
}

bool IOManager::exportProject(const ProjectFile& project, const std::string& file_path) {
    try {
        json project_json;

        project_json["width"] = project.width();
        project_json["height"] = project.height();

        json layers_json = json::array();

        // Export each layer
        for (const auto& layer : project.layers()) {
            json layer_json;
            layer_json["name"] = layer->name();
            layer_json["visible"] = layer->visible();
            layer_json["opacity"] = layer->opacity();
            layer_json["blend_mode"] = blend_mode_to_string(layer->blend_mode());
            layer_json["width"] = layer->width();
            layer_json["height"] = layer->height();
            layer_json["data"] = layer->data();

            layers_json.push_back(layer_json);
        }

        project_json["layers"] = layers_json;

        std::ofstream output_file(file_path);
        if (!output_file.is_open()) {
            return false;
        }

        output_file << project_json.dump(4);  // Pretty-print with 4-space indent
        output_file.close();

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error exporting project: " << e.what() << std::endl;
        return false;
    }
}

ImageFile IOManager::readImage(const std::string& file_path) {
    cv::Mat img = cv::imread(file_path, cv::IMREAD_UNCHANGED);
    return ImageFile(img, file_path);
}

bool IOManager::writeImage(const cv::Mat& mat, const std::string& file_path) {
    return cv::imwrite(file_path, mat);
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
