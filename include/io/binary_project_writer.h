/**
 * @file binary_project_writer.h
 * @brief Binary project file writer with LZ4 compression.
 * @author Laurent Jiang
 * @date 2026-02-11
 */

#pragma once

#include "core/document.h"
#include "error_handling/error_result.h"

#include <filesystem>

namespace gimp {

/*!
 * @class BinaryProjectWriter
 * @brief Writes Document to binary .gimp format with LZ4 compression.
 *
 * File format specification:
 * - Header (16 bytes): Magic "GIMP", version, width, height
 * - Chunk table: count + entries (type, offset, compressed_size, uncompressed_size)
 * - Layer chunks: name, visibility, opacity, blend mode, LZ4-compressed RGBA
 * - Selection chunk: serialized QPainterPath elements
 */
class BinaryProjectWriter {
  public:
    /*!
     * @brief Writes a document to a binary project file.
     * @param doc The document to write.
     * @param path The file path to write to.
     * @return Result<void> indicating success or failure.
     */
    static error::Result<void> write(const Document& doc, const std::filesystem::path& path);

  private:
    static constexpr uint32_t kMagic = 0x504D4947;  // "GIMP" in little-endian
    static constexpr uint32_t kVersion = 1;
};

}  // namespace gimp
