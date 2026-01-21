/**
 * @file io_manager.cpp
 * @brief Implementation of IOManager for file I/O and project import/export.
 * @author Aless Tosi
 * @date 2025-12-17
 */

#include "io/io_manager.h"

#include "io/opencv_image.h"

#include <nlohmann/json.hpp>
#include <opencv2/imgproc.hpp>

#include <fstream>

namespace gimp {

IOManager::IOManager() = default;
IOManager::~IOManager() = default;

std::shared_ptr<Image> IOManager::read_image(const std::string& file_path)
{
    const cv::Mat img = cv::imread(file_path, cv::IMREAD_UNCHANGED);

    if (img.empty())
        return nullptr;
    return std::make_shared<OpenCVImage>(img, file_path);
}

bool IOManager::write_image(const Image& image, const std::string& file_path)
{
    return cv::imwrite(file_path, image.mat());
}

std::shared_ptr<Document> IOManager::import_project(const std::string& file_path)
{
    // Placeholder: implement your custom project format here
    // Example: read JSON, reconstruct Document
    std::ifstream in(file_path);
    if (!in)
        return nullptr;
    nlohmann::json j;
    in >> j;
    // ... reconstruct Document from j ...
    return nullptr;  // Not implemented
}

bool IOManager::export_project(const Document& document, const std::string& file_path)
{
    // Placeholder: implement your custom project format here
    // Example: serialize Document to JSON
    (void)document;  // TODO: implement serialization
    const nlohmann::json j;
    // ... serialize Document to j ...
    std::ofstream out(file_path);
    if (!out)
        return false;
    out << j.dump(2);
    return true;  // Not implemented
}

}  // namespace gimp
