/**
 * @file binary_project_reader.cpp
 * @brief Binary project file reader implementation.
 * @author Laurent Jiang
 * @date 2026-02-11
 */

#include "io/binary_project_reader.h"

#include "core/layer.h"

#include <array>
#include <cstring>
#include <fstream>
#include <vector>

#include <lz4.h>

namespace gimp {

namespace {

// Chunk type identifiers (4 bytes each)
constexpr std::array<char, 4> kChunkTypeLayer = {'L', 'A', 'Y', 'R'};
constexpr std::array<char, 4> kChunkTypeSelection = {'S', 'E', 'L', 'C'};

// Chunk table entry
struct ChunkEntry {
    std::array<char, 4> type;
    uint32_t offset;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
};

// Helper to read little-endian uint32
uint32_t readUint32(std::istream& in)
{
    uint32_t value = 0;
    in.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

// Helper to read little-endian float
float readFloat(std::istream& in)
{
    float value = 0;
    in.read(reinterpret_cast<char*>(&value), sizeof(value));
    return value;
}

// Decompress data with LZ4
std::vector<uint8_t> decompressLZ4(const std::vector<char>& compressed, size_t uncompressedSize)
{
    std::vector<uint8_t> decompressed(uncompressedSize);

    const int result = LZ4_decompress_safe(compressed.data(),
                                           reinterpret_cast<char*>(decompressed.data()),
                                           static_cast<int>(compressed.size()),
                                           static_cast<int>(uncompressedSize));

    if (result < 0) {
        decompressed.clear();  // Signal decompression failure
    }

    return decompressed;
}

// Deserialize a layer from bytes
std::shared_ptr<Layer> deserializeLayer(const std::vector<uint8_t>& data)
{
    if (data.size() < 4) {
        return nullptr;
    }

    size_t offset = 0;

    // Name length
    uint32_t nameLen = 0;
    std::memcpy(&nameLen, data.data() + offset, sizeof(nameLen));
    offset += sizeof(nameLen);

    if (offset + nameLen > data.size()) {
        return nullptr;
    }

    // Name
    std::string name(reinterpret_cast<const char*>(data.data() + offset), nameLen);
    offset += nameLen;

    if (offset + 1 + 4 + 1 + 8 > data.size()) {
        return nullptr;
    }

    // Visible (1 byte)
    const bool visible = data[offset] != 0;
    offset += 1;

    // Opacity (4 bytes)
    float opacity = 0;
    std::memcpy(&opacity, data.data() + offset, sizeof(opacity));
    offset += sizeof(opacity);

    // Blend mode (1 byte)
    const auto blendMode = static_cast<BlendMode>(data[offset]);
    offset += 1;

    // Layer dimensions
    uint32_t layerWidth = 0;
    uint32_t layerHeight = 0;
    std::memcpy(&layerWidth, data.data() + offset, sizeof(layerWidth));
    offset += sizeof(layerWidth);
    std::memcpy(&layerHeight, data.data() + offset, sizeof(layerHeight));
    offset += sizeof(layerHeight);

    // Verify pixel data size
    const size_t expectedPixelSize = static_cast<size_t>(layerWidth) * layerHeight * 4;
    if (offset + expectedPixelSize > data.size()) {
        return nullptr;
    }

    // Create layer and copy pixel data
    auto layer =
        std::make_shared<Layer>(static_cast<int>(layerWidth), static_cast<int>(layerHeight));
    layer->setName(name);
    layer->setVisible(visible);
    layer->setOpacity(opacity);
    layer->setBlendMode(blendMode);

    std::memcpy(layer->data().data(), data.data() + offset, expectedPixelSize);

    return layer;
}

// Deserialize selection path from bytes
QPainterPath deserializeSelection(const std::vector<uint8_t>& data)
{
    QPainterPath path;

    if (data.size() < 4) {
        return path;
    }

    size_t offset = 0;

    // Element count
    uint32_t count = 0;
    std::memcpy(&count, data.data() + offset, sizeof(count));
    offset += sizeof(count);

    for (uint32_t i = 0; i < count && offset + 9 <= data.size(); ++i) {
        // Element type (1 byte)
        const auto elemType = static_cast<QPainterPath::ElementType>(data[offset]);
        offset += 1;

        // X, Y coordinates (8 bytes)
        float x = 0;
        float y = 0;
        std::memcpy(&x, data.data() + offset, sizeof(x));
        offset += sizeof(x);
        std::memcpy(&y, data.data() + offset, sizeof(y));
        offset += sizeof(y);

        switch (elemType) {
            case QPainterPath::MoveToElement:
                path.moveTo(static_cast<qreal>(x), static_cast<qreal>(y));
                break;
            case QPainterPath::LineToElement:
                path.lineTo(static_cast<qreal>(x), static_cast<qreal>(y));
                break;
            case QPainterPath::CurveToElement:
                // For curves, we need control points which follow
                // This is a simplified implementation - full curves would need more data
                path.lineTo(static_cast<qreal>(x), static_cast<qreal>(y));
                break;
            case QPainterPath::CurveToDataElement:
                // Skip curve data elements as they're handled with CurveToElement
                break;
        }
    }

    return path;
}

}  // namespace

error::Result<std::shared_ptr<ProjectFile>> BinaryProjectReader::read(
    const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return error::ErrorInfo(error::ErrorCode::IOFileNotFound,
                                "Failed to open file: " + path.string());
    }

    // --- Read Header (16 bytes) ---
    const uint32_t magic = readUint32(file);
    if (magic != kMagic) {
        return error::ErrorInfo(error::ErrorCode::IOCorruptedFile,
                                "Invalid magic header: not a GIMP project file");
    }

    const uint32_t version = readUint32(file);
    if (version > kSupportedVersion) {
        return error::ErrorInfo(error::ErrorCode::IOUnsupportedFormat,
                                "Unsupported file version: " + std::to_string(version));
    }

    const uint32_t docWidth = readUint32(file);
    const uint32_t docHeight = readUint32(file);

    if (!file.good()) {
        return error::ErrorInfo(error::ErrorCode::IOCorruptedFile, "Failed to read file header");
    }

    // Create the document
    auto document =
        std::make_shared<ProjectFile>(static_cast<int>(docWidth), static_cast<int>(docHeight));
    document->setFilePath(path);

    // --- Read Chunk Table ---
    const uint32_t chunkCount = readUint32(file);

    std::vector<ChunkEntry> chunkTable;
    chunkTable.reserve(chunkCount);

    for (uint32_t i = 0; i < chunkCount; ++i) {
        ChunkEntry entry{};
        file.read(entry.type.data(), 4);
        entry.offset = readUint32(file);
        entry.compressedSize = readUint32(file);
        entry.uncompressedSize = readUint32(file);
        chunkTable.push_back(entry);
    }

    if (!file.good()) {
        return error::ErrorInfo(error::ErrorCode::IOCorruptedFile, "Failed to read chunk table");
    }

    // --- Read Chunk Data ---
    for (const auto& chunk : chunkTable) {
        file.seekg(chunk.offset, std::ios::beg);

        std::vector<char> compressed(chunk.compressedSize);
        file.read(compressed.data(), static_cast<std::streamsize>(chunk.compressedSize));

        if (!file.good()) {
            return error::ErrorInfo(
                error::ErrorCode::IOReadError,
                "Failed to read chunk data at offset " + std::to_string(chunk.offset));
        }

        std::vector<uint8_t> decompressed = decompressLZ4(compressed, chunk.uncompressedSize);
        if (decompressed.empty() && chunk.uncompressedSize > 0) {
            return error::ErrorInfo(error::ErrorCode::IOCorruptedFile,
                                    "LZ4 decompression failed for chunk");
        }

        if (chunk.type == kChunkTypeLayer) {
            auto layer = deserializeLayer(decompressed);
            if (!layer) {
                return error::ErrorInfo(error::ErrorCode::IOCorruptedFile,
                                        "Failed to deserialize layer");
            }

            // Add layer by copying data to the document's layer
            auto docLayer = document->addLayer();
            docLayer->setName(layer->name());
            docLayer->setVisible(layer->visible());
            docLayer->setOpacity(layer->opacity());
            docLayer->setBlendMode(layer->blendMode());

            // Copy pixel data
            if (layer->width() == docLayer->width() && layer->height() == docLayer->height()) {
                std::memcpy(docLayer->data().data(), layer->data().data(), layer->data().size());
            }
        } else if (chunk.type == kChunkTypeSelection) {
            QPainterPath selection = deserializeSelection(decompressed);
            document->setSelectionPath(selection);
        }
        // Unknown chunk types are silently skipped for forward compatibility
    }

    return document;
}

}  // namespace gimp
