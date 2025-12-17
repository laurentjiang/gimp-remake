/**
 * @file io_interface.h
 * @brief Interface for main module for file reading and writing, project import and export.
 * @author Aless Tosi
 * @date 2025-12-17
 */

#pragma once

#include <memory>
#include <string>

#include "../core/image.h"
#include "../core/document.h"

namespace gimp {

class IOInterface {
  public:
    virtual ~IOInterface() = default;

    // Load a single-layer image file (PNG, JPEG, etc.)
    virtual std::shared_ptr<Image> read_image(const std::string& file_path) = 0;

    // Save a single-layer image file
    virtual bool write_image(const Image& image, const std::string& file_path) = 0;

    // Import a multi-layer project file (custom format, e.g., .gimp)
    virtual std::shared_ptr<Document> import_project(const std::string& file_path) = 0;

    // Export a multi-layer project file
    virtual bool export_project(const Document& document, const std::string& file_path) = 0;
};

}  // namespace gimp
