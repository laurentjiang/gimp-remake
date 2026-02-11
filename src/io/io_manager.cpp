/**
 * @file io_manager.cpp
 * @brief Implementation of IOManager for file I/O and project import/export.
 * @author Aless Tosi
 * @date 2025-12-17
 */

#include "io/io_manager.h"

#include "io/binary_project_reader.h"
#include "io/binary_project_writer.h"
#include "io/utility.h"

#include <QPainterPath>
#include <QPointF>

#include <nlohmann/json.hpp>
#include <opencv2/imgproc.hpp>

#include <cstring>
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

// Magic bytes for binary format detection
constexpr uint32_t kBinaryMagic = 0x504D4947;  // "GIMP" in little-endian

bool isBinaryFormat(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    uint32_t magic = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    return magic == kBinaryMagic;
}

}  // namespace

error::Result<void> IOManager::saveProject(const Document& doc, const std::filesystem::path& path)
{
    return BinaryProjectWriter::write(doc, path);
}

error::Result<std::shared_ptr<ProjectFile>> IOManager::loadProject(
    const std::filesystem::path& path)
{
    if (!std::filesystem::exists(path)) {
        return error::ErrorInfo(error::ErrorCode::IOFileNotFound,
                                "File not found: " + path.string());
    }

    // Check if it's binary format by reading magic header
    if (isBinaryFormat(path)) {
        return BinaryProjectReader::read(path);
    }

    // Fall back to legacy JSON import
    try {
        ProjectFile project = importProject(path.string());
        auto result = std::make_shared<ProjectFile>(project.width(), project.height());
        result->setFilePath(path);
        result->setSelectionPath(project.selectionPath());

        // Copy layers
        for (int i = 0; i < project.layers().count(); ++i) {
            const auto& srcLayer = project.layers()[i];
            auto dstLayer = result->addLayer();
            dstLayer->setName(srcLayer->name());
            dstLayer->setVisible(srcLayer->visible());
            dstLayer->setOpacity(srcLayer->opacity());
            dstLayer->setBlendMode(srcLayer->blendMode());
            std::memcpy(dstLayer->data().data(), srcLayer->data().data(), srcLayer->data().size());
        }

        return result;
    } catch (const std::exception& e) {
        return error::ErrorInfo(error::ErrorCode::IOCorruptedFile, e.what());
    }
}

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

    if (projectJson.contains("selection")) {
        project.setSelectionPath(deserializeSelectionPath(projectJson.at("selection")));
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
