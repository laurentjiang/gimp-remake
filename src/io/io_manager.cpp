/**
 * @file io_manager.cpp
 * @brief Implementation of IOManager for file I/O and project import/export.
 * @author Aless Tosi
 * @date 2025-12-17
 */

#include <fstream>
#include <nlohmann/json.hpp>
#include <opencv2/imgproc.hpp>
#include <stdexcept>

#include "io/io_manager.h"

namespace gimp {

/* DocumentFile IOManager::importProject(const std::string& file_path) {
    // Placeholder: not implemented
    throw std::runtime_error("importProject not implemented");
}

bool IOManager::exportProject(const Document& document, const std::string& file_path) {
    // Placeholder: implement your custom project format here
    // Example: serialize Document to JSON
    nlohmann::json j;
    // ... serialize Document to j ...
    std::ofstream out(file_path);
    if (!out) return false;
    out << j.dump(2);
    return true; // Not implemented
} */

ImageFile IOManager::readImage(const std::string& file_path) {
    cv::Mat img = cv::imread(file_path, cv::IMREAD_UNCHANGED);
    return ImageFile(img, file_path);
}

bool IOManager::writeImage(const cv::Mat& mat, const std::string& file_path) {
    return cv::imwrite(file_path, mat);
}

void IOManager::toGrayscale(cv::Mat& mat)
{
    cv::Mat copy = mat.clone();
    if (mat.channels() == 3) {
        cv::cvtColor(copy, mat, cv::COLOR_RGB2GRAY);
    } else if (mat.channels() == 4) {
        cv::cvtColor(copy, mat, cv::COLOR_RGBA2GRAY);
    }
}

void IOManager::toRgb(cv::Mat& mat)
{
    cv::Mat copy = mat.clone();
    if (mat.channels() == 1) {
        cv::cvtColor(copy, mat, cv::COLOR_GRAY2RGB);
    } else if (mat.channels() == 4) {
        cv::cvtColor(copy, mat, cv::COLOR_RGBA2RGB);
    }
}

void IOManager::toRgba(cv::Mat& mat)
{
    cv::Mat copy = mat.clone();
    if (mat.channels() == 1) {
        cv::cvtColor(copy, mat, cv::COLOR_GRAY2RGBA);
    } else if (mat.channels() == 3) {
        cv::cvtColor(copy, mat, cv::COLOR_RGB2RGBA);
    }
}

} // namespace gimp
