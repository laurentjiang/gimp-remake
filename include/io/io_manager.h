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

/* #include "io/document_file.h" */
#include "io/image_file.h"

namespace gimp {

    class IOManager {
        public:
            IOManager() = default;
            ~IOManager() = default;

/*             DocumentFile importProject(const std::string& file_path);
            bool exportProject(const Document& document, const std::string& file_path); */

            ImageFile readImage(const std::string& file_path);
            bool writeImage(const cv::Mat& mat, const std::string& file_path);
            void toGrayscale(cv::Mat& mat);
            void toRgb(cv::Mat& mat);
            void toRgba(cv::Mat& mat);
    }; // class IOManager

} // namespace gimp
