/**
 * @file io_manager.h
 * @brief Default implementation of IOInterface for file I/O and project import/export.
 * @author Aless Tosi
 * @date 2025-12-17
 */

#pragma once

#include "io_interface.h"

#include <opencv2/imgcodecs.hpp>

#include <memory>
#include <string>

namespace gimp {

class IOManager : public IOInterface {
  public:
    IOManager();
    ~IOManager() override;

    std::shared_ptr<Image> read_image(const std::string& file_path) override;
    bool write_image(const Image& image, const std::string& file_path) override;

    std::shared_ptr<Document> import_project(const std::string& file_path) override;
    bool export_project(const Document& document, const std::string& file_path) override;
};

}  // namespace gimp
