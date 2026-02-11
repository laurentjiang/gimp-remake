/**
 * @file binary_project_reader.h
 * @brief Binary project file reader with LZ4 decompression.
 * @author Laurent Jiang
 * @date 2026-02-11
 */

#pragma once

#include "error_handling/error_result.h"
#include "io/project_file.h"

#include <filesystem>
#include <memory>

namespace gimp {

/*!
 * @class BinaryProjectReader
 * @brief Reads binary .gimp format files with LZ4 decompression.
 *
 * Validates magic header and version, decompresses layer data,
 * and reconstructs the Document with all layers and selection.
 */
class BinaryProjectReader {
  public:
    /*!
     * @brief Reads a binary project file.
     * @param path The file path to read from.
     * @return Result containing the loaded ProjectFile or an error.
     */
    static error::Result<std::shared_ptr<ProjectFile>> read(const std::filesystem::path& path);

  private:
    static constexpr uint32_t kMagic = 0x504D4947;       // "GIMP" in little-endian
    static constexpr uint32_t kSupportedVersion = 1;
};

}  // namespace gimp
