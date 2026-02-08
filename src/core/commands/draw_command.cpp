/**
 * @file draw_command.cpp
 * @brief Command stub to draw on a layer.
 * @author Aless Tosi
 * @date 2026-01-25
 */

#include "core/commands/draw_command.h"

#include "core/layer.h"

#include <algorithm>
#include <cstring>

namespace gimp {

DrawCommand::DrawCommand(std::shared_ptr<Layer> layer, int x, int y, int width, int height)
    : layer_{std::move(layer)},
      regionX_{x},
      regionY_{y},
      regionWidth_{width},
      regionHeight_{height}
{
}

void DrawCommand::captureBeforeState()
{
    if (!layer_) {
        return;
    }

    // Calculate the actual clipped region within layer bounds
    int clippedX = std::max(0, regionX_);
    int clippedY = std::max(0, regionY_);
    int clippedWidth = std::min(regionWidth_, layer_->width() - clippedX);
    int clippedHeight = std::min(regionHeight_, layer_->height() - clippedY);

    if (clippedWidth <= 0 || clippedHeight <= 0) {
        beforeState_.clear();
        return;
    }

    // Allocate space for the region (RGBA = 4 bytes per pixel)
    beforeState_.resize(static_cast<std::size_t>(clippedWidth * clippedHeight) * 4);

    const auto& layerData = layer_->data();
    const int layerWidth = layer_->width();
    const int pixelSize = 4;  // RGBA

    // Copy the region from the layer
    for (int row = 0; row < clippedHeight; ++row) {
        const int srcRow = clippedY + row;
        const int srcOffset = (srcRow * layerWidth + clippedX) * pixelSize;
        const int dstOffset = row * clippedWidth * pixelSize;

        std::memcpy(beforeState_.data() + dstOffset,
                    layerData.data() + srcOffset,
                    (clippedWidth * pixelSize));
    }
}

void DrawCommand::captureAfterState()
{
    if (!layer_) {
        return;
    }

    // Calculate the actual clipped region within layer bounds
    int clippedX = std::max(0, regionX_);
    int clippedY = std::max(0, regionY_);
    int clippedWidth = std::min(regionWidth_, layer_->width() - clippedX);
    int clippedHeight = std::min(regionHeight_, layer_->height() - clippedY);

    if (clippedWidth <= 0 || clippedHeight <= 0) {
        afterState_.clear();
        return;
    }

    // Allocate space for the region (RGBA = 4 bytes per pixel)
    afterState_.resize(static_cast<std::size_t>(clippedWidth * clippedHeight) * 4);

    const auto& layerData = layer_->data();
    const int layerWidth = layer_->width();
    const int pixelSize = 4;  // RGBA

    // Copy the region from the layer
    for (int row = 0; row < clippedHeight; ++row) {
        const int srcRow = clippedY + row;
        const int srcOffset = (srcRow * layerWidth + clippedX) * pixelSize;
        const int dstOffset = row * clippedWidth * pixelSize;

        std::memcpy(afterState_.data() + dstOffset,
                    layerData.data() + srcOffset,
                    (clippedWidth * pixelSize));
    }
}

void DrawCommand::apply()
{
    updateState(afterState_);
}

void DrawCommand::undo()
{
    updateState(beforeState_);
}

void DrawCommand::updateState(const std::vector<std::uint8_t>& state)
{
    if (!layer_ || state.empty()) {
        return;
    }

    int clippedX = std::max(0, regionX_);
    int clippedY = std::max(0, regionY_);
    int clippedWidth = std::min(regionWidth_, layer_->width() - clippedX);
    int clippedHeight = std::min(regionHeight_, layer_->height() - clippedY);

    if (clippedWidth <= 0 || clippedHeight <= 0) {
        return;
    }

    auto& layerData = layer_->data();
    const int layerWidth = layer_->width();
    const int pixelSize = 4;  // RGBA

    // Restore the region to the layer
    for (int row = 0; row < clippedHeight; ++row) {
        const int dstRow = clippedY + row;
        const int dstOffset = (dstRow * layerWidth + clippedX) * pixelSize;
        const int srcOffset = row * clippedWidth * pixelSize;

        std::memcpy(
            layerData.data() + dstOffset, state.data() + srcOffset, (clippedWidth * pixelSize));
    }
}

}  // namespace gimp
