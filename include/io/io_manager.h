/**
 * @file io_manager.h
 * @brief Default implementation of IOInterface for file I/O and project import/export.
 * @author Aless Tosi
 * @date 2025-12-17
 */

#pragma once

#include <memory>
#include <string>
#include <opencv2/imgcodecs.hpp>

#include "io/project_file.h"
#include "io/image_file.h"

namespace gimp {

    class IOManager {
        public:
            IOManager() = default;
            ~IOManager() = default;

            ProjectFile importProject(const std::string& file_path);
            bool exportProject(const ProjectFile& project, const std::string& file_path);

            ImageFile readImage(const std::string& input_path);
            bool writeImage(const cv::Mat& image, const std::string& output_path);
            void toGrayscale(cv::Mat& image);
            void toRgb(cv::Mat& image);
            void toRgba(cv::Mat& image);
    }; // class IOManager

} // namespace gimp
