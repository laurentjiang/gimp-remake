/**
 * @file io_manager.h
 * @brief Default implementation of IOInterface for file I/O and project import/export.
 * @author Aless Tosi
 * @date 2025-12-17
 */

#pragma once

#include "io/image_file.h"
#include "io/project_file.h"

#include <opencv2/imgcodecs.hpp>

#include <memory>
#include <string>

namespace gimp {

class IOManager {
  public:
    IOManager() = default;
    ~IOManager() = default;

    ProjectFile importProject(const std::string& file_path);
    bool exportProject(const ProjectFile& project, const std::string& file_path);

    ImageFile readImage(const std::string& file_path);
    bool writeImage(const cv::Mat& mat, const std::string& file_path);
    void toGrayscale(cv::Mat& mat);
    void toRgb(cv::Mat& mat);
    void toRgba(cv::Mat& mat);
};  // class IOManager

}  // namespace gimp
