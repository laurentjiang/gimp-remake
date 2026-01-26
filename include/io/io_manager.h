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

/*!
 * @class IOManager
 * @brief Handles file I/O operations for images and project files.
 */
class IOManager {
  public:
    IOManager() = default;
    ~IOManager() = default;

    /*!
     * @brief Imports a project from a JSON file.
     * @param file_path Path to the project file.
     * @return The loaded ProjectFile.
     * @throws std::runtime_error If file cannot be opened or data is invalid.
     */
    ProjectFile importProject(const std::string& file_path);

    /*!
     * @brief Exports a project to a JSON file.
     * @param project The project to export.
     * @param file_path Destination file path.
     * @return True on success, false on failure.
     */
    bool exportProject(const ProjectFile& project, const std::string& file_path);

    /*!
     * @brief Reads an image file from disk.
     * @param file_path Path to the image file.
     * @return ImageFile containing the loaded image data.
     * @pre file_path must be a valid, accessible file path.
     * @post If successful, returned ImageFile::empty() is false.
     */
    ImageFile readImage(const std::string& file_path);

    /*!
     * @brief Writes an image to disk.
     * @param mat The image matrix to write.
     * @param file_path Destination file path.
     * @return True on success, false on failure.
     * @pre mat must not be empty.
     * @pre file_path must be a writable location.
     */
    bool writeImage(const cv::Mat& mat, const std::string& file_path);

    /*!
     * @brief Converts an image to grayscale in-place.
     * @param mat The image to convert (modified in-place).
     */
    void toGrayscale(cv::Mat& mat);

    /*!
     * @brief Converts an image to RGB in-place.
     * @param mat The image to convert (modified in-place).
     */
    void toRgb(cv::Mat& mat);

    /*!
     * @brief Converts an image to RGBA in-place.
     * @param mat The image to convert (modified in-place).
     */
    void toRgba(cv::Mat& mat);
};  // class IOManager

}  // namespace gimp
