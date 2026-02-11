/**
 * @file binary_project_writer.cpp
 * @brief Binary project file writer implementation.
 * @author Laurent Jiang
 * @date 2026-02-11
 */

#include "io/binary_project_writer.h"

#include "core/layer.h"

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

// Helper to write little-endian uint32
void writeUint32(std::ostream& out, uint32_t value)
{
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

// Helper to write little-endian float
void writeFloat(std::ostream& out, float value)
{
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

// Compress data with LZ4
std::vector<char> compressLZ4(const std::vector<uint8_t>& data)
{
    const int maxDstSize = LZ4_compressBound(static_cast<int>(data.size()));
    std::vector<char> compressed(static_cast<size_t>(maxDstSize));

    const int compressedSize = LZ4_compress_default(reinterpret_cast<const char*>(data.data()),
                                                    compressed.data(),
                                                    static_cast<int>(data.size()),
                                                    maxDstSize);

    compressed.resize(static_cast<size_t>(compressedSize));
    return compressed;
}

// Serialize a single layer to bytes
std::vector<uint8_t> serializeLayer(const Layer& layer)
{
    std::vector<uint8_t> buffer;

    // Name (length + UTF-8 bytes)
    const std::string& name = layer.name();
    const auto nameLen = static_cast<uint32_t>(name.size());
    buffer.insert(buffer.end(),
                  reinterpret_cast<const uint8_t*>(&nameLen),
                  reinterpret_cast<const uint8_t*>(&nameLen) + sizeof(nameLen));
    buffer.insert(buffer.end(), name.begin(), name.end());

    // Visible (1 byte)
    const uint8_t visible = layer.visible() ? 1 : 0;
    buffer.push_back(visible);

    // Opacity (4 bytes float)
    const float opacity = layer.opacity();
    buffer.insert(buffer.end(),
                  reinterpret_cast<const uint8_t*>(&opacity),
                  reinterpret_cast<const uint8_t*>(&opacity) + sizeof(opacity));

    // Blend mode (1 byte)
    const auto blendMode = static_cast<uint8_t>(layer.blendMode());
    buffer.push_back(blendMode);

    // Layer dimensions (8 bytes)
    const auto layerWidth = static_cast<uint32_t>(layer.width());
    const auto layerHeight = static_cast<uint32_t>(layer.height());
    buffer.insert(buffer.end(),
                  reinterpret_cast<const uint8_t*>(&layerWidth),
                  reinterpret_cast<const uint8_t*>(&layerWidth) + sizeof(layerWidth));
    buffer.insert(buffer.end(),
                  reinterpret_cast<const uint8_t*>(&layerHeight),
                  reinterpret_cast<const uint8_t*>(&layerHeight) + sizeof(layerHeight));

    // RGBA pixel data (uncompressed in this buffer, will be LZ4 compressed later)
    const auto& pixelData = layer.data();
    buffer.insert(buffer.end(), pixelData.begin(), pixelData.end());

    return buffer;
}

// Serialize selection path to bytes
std::vector<uint8_t> serializeSelection(const QPainterPath& path)
{
    std::vector<uint8_t> buffer;

    const int elementCount = path.elementCount();
    const auto count = static_cast<uint32_t>(elementCount);
    buffer.insert(buffer.end(),
                  reinterpret_cast<const uint8_t*>(&count),
                  reinterpret_cast<const uint8_t*>(&count) + sizeof(count));

    for (int i = 0; i < elementCount; ++i) {
        const QPainterPath::Element elem = path.elementAt(i);

        // Element type (1 byte): MoveTo=0, LineTo=1, CurveTo=2
        const auto elemType = static_cast<uint8_t>(elem.type);
        buffer.push_back(elemType);

        // X, Y coordinates (8 bytes)
        const auto x = static_cast<float>(elem.x);
        const auto y = static_cast<float>(elem.y);
        buffer.insert(buffer.end(),
                      reinterpret_cast<const uint8_t*>(&x),
                      reinterpret_cast<const uint8_t*>(&x) + sizeof(x));
        buffer.insert(buffer.end(),
                      reinterpret_cast<const uint8_t*>(&y),
                      reinterpret_cast<const uint8_t*>(&y) + sizeof(y));
    }

    return buffer;
}

}  // namespace

error::Result<void> BinaryProjectWriter::write(const Document& doc,
                                               const std::filesystem::path& path)
{
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return error::Err(error::ErrorCode::IOWriteError,
                          "Failed to open file for writing: " + path.string());
    }

    // --- Write Header (16 bytes) ---
    writeUint32(file, kMagic);
    writeUint32(file, kVersion);
    writeUint32(file, static_cast<uint32_t>(doc.width()));
    writeUint32(file, static_cast<uint32_t>(doc.height()));

    // --- Prepare chunks ---
    std::vector<ChunkEntry> chunkTable;
    std::vector<std::vector<char>> compressedChunks;

    // Layer chunks
    const LayerStack& layers = doc.layers();
    for (int i = 0; i < layers.count(); ++i) {
        const auto& layer = layers[i];
        std::vector<uint8_t> layerData = serializeLayer(*layer);
        std::vector<char> compressed = compressLZ4(layerData);

        if (compressed.empty() && !layerData.empty()) {
            return error::Err(error::ErrorCode::IOWriteError, "LZ4 compression failed for layer");
        }

        ChunkEntry entry{};
        entry.type = kChunkTypeLayer;
        entry.offset = 0;  // Will be computed after chunk table size is known
        entry.compressedSize = static_cast<uint32_t>(compressed.size());
        entry.uncompressedSize = static_cast<uint32_t>(layerData.size());

        chunkTable.push_back(entry);
        compressedChunks.push_back(std::move(compressed));
    }

    // Selection chunk (if not empty)
    QPainterPath selection = doc.selectionPath();
    if (!selection.isEmpty()) {
        std::vector<uint8_t> selectionData = serializeSelection(selection);
        std::vector<char> compressed = compressLZ4(selectionData);

        ChunkEntry entry{};
        entry.type = kChunkTypeSelection;
        entry.offset = 0;
        entry.compressedSize = static_cast<uint32_t>(compressed.size());
        entry.uncompressedSize = static_cast<uint32_t>(selectionData.size());

        chunkTable.push_back(entry);
        compressedChunks.push_back(std::move(compressed));
    }

    // --- Write Chunk Table ---
    const auto chunkCount = static_cast<uint32_t>(chunkTable.size());
    writeUint32(file, chunkCount);

    // Calculate chunk data start offset
    // Header (16) + chunk count (4) + chunk entries (16 each)
    uint32_t dataOffset = 16 + 4 + (chunkCount * 16);

    // Update offsets and write entries
    for (size_t i = 0; i < chunkTable.size(); ++i) {
        chunkTable[i].offset = dataOffset;
        dataOffset += chunkTable[i].compressedSize;

        file.write(chunkTable[i].type.data(), 4);
        writeUint32(file, chunkTable[i].offset);
        writeUint32(file, chunkTable[i].compressedSize);
        writeUint32(file, chunkTable[i].uncompressedSize);
    }

    // --- Write Chunk Data ---
    for (const auto& chunk : compressedChunks) {
        file.write(chunk.data(), static_cast<std::streamsize>(chunk.size()));
    }

    if (!file.good()) {
        return error::Err(error::ErrorCode::IOWriteError,
                          "Failed to write project file: " + path.string());
    }

    return error::Ok();
}

}  // namespace gimp
