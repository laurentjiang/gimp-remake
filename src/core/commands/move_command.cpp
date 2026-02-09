/**
 * @file move_command.cpp
 * @brief Implementation of MoveCommand.
 * @author Laurent Jiang
 * @date 2026-02-09
 */

#include "core/commands/move_command.h"

#include "core/event_bus.h"
#include "core/events.h"
#include "core/layer.h"
#include "core/selection_manager.h"

#include <algorithm>
#include <cstring>

namespace gimp {

MoveCommand::MoveCommand(std::shared_ptr<Layer> layer, QRect affectedRegion)
    : layer_{std::move(layer)},
      affectedRegion_{affectedRegion}
{
}

void MoveCommand::captureBeforeState()
{
    if (!layer_) {
        return;
    }

    // Capture selection state
    beforeSelectionPath_ = SelectionManager::instance().selectionPath();
    beforeSelectionType_ = SelectionManager::instance().selectionType();

    // Calculate the actual clipped region within layer bounds
    int clippedX = std::max(0, affectedRegion_.x());
    int clippedY = std::max(0, affectedRegion_.y());
    int clippedRight = std::min(affectedRegion_.x() + affectedRegion_.width(), layer_->width());
    int clippedBottom = std::min(affectedRegion_.y() + affectedRegion_.height(), layer_->height());
    int clippedWidth = clippedRight - clippedX;
    int clippedHeight = clippedBottom - clippedY;

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
                    static_cast<std::size_t>(clippedWidth) * pixelSize);
    }
}

void MoveCommand::captureAfterState()
{
    if (!layer_) {
        return;
    }

    // Capture selection state
    afterSelectionPath_ = SelectionManager::instance().selectionPath();
    afterSelectionType_ = SelectionManager::instance().selectionType();

    // Calculate the actual clipped region within layer bounds
    int clippedX = std::max(0, affectedRegion_.x());
    int clippedY = std::max(0, affectedRegion_.y());
    int clippedRight = std::min(affectedRegion_.x() + affectedRegion_.width(), layer_->width());
    int clippedBottom = std::min(affectedRegion_.y() + affectedRegion_.height(), layer_->height());
    int clippedWidth = clippedRight - clippedX;
    int clippedHeight = clippedBottom - clippedY;

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
                    static_cast<std::size_t>(clippedWidth) * pixelSize);
    }
}

void MoveCommand::apply()
{
    updateState(afterState_);
    restoreSelection(afterSelectionPath_, afterSelectionType_);
}

void MoveCommand::undo()
{
    updateState(beforeState_);
    restoreSelection(beforeSelectionPath_, beforeSelectionType_);
}

void MoveCommand::updateState(const std::vector<std::uint8_t>& state)
{
    if (!layer_ || state.empty()) {
        return;
    }

    int clippedX = std::max(0, affectedRegion_.x());
    int clippedY = std::max(0, affectedRegion_.y());
    int clippedRight = std::min(affectedRegion_.x() + affectedRegion_.width(), layer_->width());
    int clippedBottom = std::min(affectedRegion_.y() + affectedRegion_.height(), layer_->height());
    int clippedWidth = clippedRight - clippedX;
    int clippedHeight = clippedBottom - clippedY;

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

        std::memcpy(layerData.data() + dstOffset,
                    state.data() + srcOffset,
                    static_cast<std::size_t>(clippedWidth) * pixelSize);
    }
}

void MoveCommand::restoreSelection(const QPainterPath& path, SelectionType type)
{
    SelectionManager::instance().clear();
    if (!path.isEmpty()) {
        SelectionManager::instance().applySelection(path, SelectionMode::Replace, type);
    }
    // Publish selection changed event so UI updates
    // NOLINTNEXTLINE(modernize-use-designated-initializers)
    EventBus::instance().publish(SelectionChangedEvent{!path.isEmpty(), "undo"});
}

}  // namespace gimp
