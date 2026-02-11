/**
 * @file io_manager.h
 * @brief Default implementation of IOInterface for file I/O and project import/export.
 * @author Aless Tosi
 * @date 2025-12-17
 */

#pragma once

#include "io/image_file.h"
#include "io/project_file.h"

#include "error_handling/error_result.h"

#include <opencv2/imgcodecs.hpp>

#include <filesystem>
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

    // ====== New Binary Format API ======

    /*!
     * @brief Saves a project to binary .gimp format with LZ4 compression.
     * @param doc The document to save.
     * @param path Destination file path.
     * @return Result<void> indicating success or failure.
     */
    error::Result<void> saveProject(const Document& doc, const std::filesystem::path& path);

    /*!
     * @brief Loads a project from file, auto-detecting format (binary or JSON).
     * @param path Path to the project file.
     * @return Result containing the loaded ProjectFile or an error.
     */
    error::Result<std::shared_ptr<ProjectFile>> loadProject(const std::filesystem::path& path);

    // ====== Legacy JSON API (for backward compatibility) ======

    /*!
     * @brief Imports a project from a JSON file.
     * @param filePath Path to the project file.
     * @return The loaded ProjectFile.
     * @throws std::runtime_error If file cannot be opened or data is invalid.
     * @deprecated Use loadProject() instead.
     */
    ProjectFile importProject(const std::string& filePath);

    /*!
     * @brief Exports a project to a JSON file.
     * @param project The project to export.
     * @param filePath Destination file path.
     * @return True on success, false on failure.
     * @deprecated Use saveProject() instead for new files.
     */
    bool exportProject(const ProjectFile& project, const std::string& filePath);

    /*!
     * @brief Reads an image file from disk.
     * @param filePath Path to the image file.
     * @return ImageFile containing the loaded image data.
     * @pre filePath must be a valid, accessible file path.
     * @post If successful, returned ImageFile::empty() is false.
     */
    ImageFile readImage(const std::string& filePath);

    /*!
     * @brief Writes an image to disk.
     * @param mat The image matrix to write.
     * @param filePath Destination file path.
     * @return True on success, false on failure.
     * @pre mat must not be empty.
     * @pre filePath must be a writable location.
     */
    bool writeImage(const cv::Mat& mat, const std::string& filePath);

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
