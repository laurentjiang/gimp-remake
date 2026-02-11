/**
 * @file io_manager.cpp
 * @brief Implementation of IOManager for file I/O and project import/export.
 * @author Aless Tosi
 * @date 2025-12-17
 */

#include "io/io_manager.h"

#include "io/utility.h"

#include <QPainterPath>
#include <QPointF>

#include <nlohmann/json.hpp>
#include <opencv2/imgproc.hpp>

#include <fstream>
#include <iostream>
#include <stdexcept>

/*! @brief Alias for nlohmann::json for convenience. */
using json = nlohmann::json;

namespace gimp {

namespace {

json serializeSelectionPath(const QPainterPath& path)
{
    json elements = json::array();
    const int count = path.elementCount();
    for (int i = 0; i < count; ++i) {
        const auto element = path.elementAt(i);
        elements.push_back({
            {"type", static_cast<int>(element.type)},
            {"x",    element.x                     },
            {"y",    element.y                     }
        });
    }
    return elements;
}

QPainterPath deserializeSelectionPath(const json& elements)
{
    QPainterPath path;
    if (!elements.is_array()) {
        return path;
    }

    std::size_t i = 0;
    while (i < elements.size()) {
        const auto& elementJson = elements.at(i);
        if (!elementJson.contains("type") || !elementJson.contains("x") ||
            !elementJson.contains("y")) {
            ++i;
            continue;
        }

        const int type = elementJson.at("type").get<int>();
        const qreal x = elementJson.at("x").get<qreal>();
        const qreal y = elementJson.at("y").get<qreal>();

        if (type == QPainterPath::MoveToElement) {
            path.moveTo(x, y);
            ++i;
        } else if (type == QPainterPath::LineToElement) {
            path.lineTo(x, y);
            ++i;
        } else if (type == QPainterPath::CurveToElement) {
            if (i + 2 >= elements.size()) {
                break;
            }

            const auto& control2Json = elements.at(i + 1);
            const auto& endJson = elements.at(i + 2);
            const qreal c2x = control2Json.at("x").get<qreal>();
            const qreal c2y = control2Json.at("y").get<qreal>();
            const qreal ex = endJson.at("x").get<qreal>();
            const qreal ey = endJson.at("y").get<qreal>();

            path.cubicTo(QPointF(x, y), QPointF(c2x, c2y), QPointF(ex, ey));
            i += 3;
        } else {
            ++i;
        }
    }

    return path;
}

}  // namespace

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
    const double dpi = projectJson.value("dpi", 72.0);

    ProjectFile project(width, height, dpi);

    // Import layers
    if (projectJson.contains("layers")) {
        for (const auto& layerJson : projectJson.at("layers")) {
            const int layerWidth = layerJson.value("width", width);
            const int layerHeight = layerJson.value("height", height);

            auto layer = project.addLayer(layerWidth, layerHeight);

            // Restore layer properties from JSON, falling back to defaults
            if (layerJson.contains("name")) {
                layer->setName(layerJson.at("name").get<std::string>());
            }
            layer->setVisible(layerJson.value("visible", true));
            layer->setOpacity(layerJson.value("opacity", 1.0F));
            if (layerJson.contains("blend_mode")) {
                layer->setBlendMode(
                    string_to_blend_mode(layerJson.at("blend_mode").get<std::string>()));
            }

            // Restore layer data when present
            if (layerJson.contains("data")) {
                const std::vector<uint8_t> layerData =
                    layerJson.at("data").get<std::vector<uint8_t>>();
                if (layerData.size() == layer->data().size()) {
                    layer->data() = layerData;
                } else {
                    throw std::runtime_error("Layer data size mismatch during import");
                }
            }
        }
    }

    if (projectJson.contains("selection")) {
        project.setSelectionPath(deserializeSelectionPath(projectJson.at("selection")));
    }

    return project;
}

bool IOManager::exportProject(const ProjectFile& project, const std::string& filePath)
{
    try {
        json projectJson;

        projectJson["version"] = 1;
        projectJson["width"] = project.width();
        projectJson["height"] = project.height();
        projectJson["dpi"] = project.dpi();

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

        if (!project.selectionPath().isEmpty()) {
            projectJson["selection"] = serializeSelectionPath(project.selectionPath());
        }

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
