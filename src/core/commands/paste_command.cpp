/**
 * @file paste_command.cpp
 * @brief Command stub to paste clipboard data into the document.
 * @author Aless Tosi
 * @date 2026-02-04
 */

#include "core/commands/paste_command.h"

#include "core/document.h"
#include "core/layer.h"

#include <algorithm>
#include <cstring>

namespace gimp {

namespace {

QImage toRgbaImage(const QImage& image)
{
    if (image.format() == QImage::Format_RGBA8888) {
        return image;
    }
    return image.convertToFormat(QImage::Format_RGBA8888);
}

}  // namespace

PasteCommand::PasteCommand(std::shared_ptr<Document> document, int x, int y, const QImage& image)
    : document_{std::move(document)},
      regionX_{x},
      regionY_{y}
{
    QImage rgbaImage = toRgbaImage(image);
    regionWidth_ = rgbaImage.width();
    regionHeight_ = rgbaImage.height();
    imageData_.resize(static_cast<std::size_t>(regionWidth_ * regionHeight_) * 4U);

    if (!rgbaImage.isNull()) {
        std::memcpy(imageData_.data(),
                    rgbaImage.bits(),
                    static_cast<std::size_t>(regionWidth_ * regionHeight_) * 4U);
    }
}

void PasteCommand::apply()
{
    if (!document_ || imageData_.empty()) {
        return;
    }

    if (!layer_) {
        // Create layer and insert above active layer (GIMP behavior)
        layer_ = std::make_shared<Layer>(document_->width(), document_->height());
        layer_->setName("Pasted Layer");

        // Insert above active layer
        std::size_t insertIndex = document_->activeLayerIndex() + 1;
        document_->layers().insertLayer(insertIndex, layer_);
        document_->setActiveLayerIndex(insertIndex);

        createdLayer_ = true;
        captured_ = false;
    }

    if (!layer_) {
        return;
    }

    if (!captured_) {
        captureBeforeState();
        writeImageToLayer();
        captureAfterState();
        captured_ = true;
        return;
    }

    updateState(afterState_);
}

void PasteCommand::undo()
{
    if (!layer_) {
        return;
    }

    if (createdLayer_ && document_) {
        document_->removeLayer(layer_);
        layer_.reset();
        createdLayer_ = false;
        captured_ = false;
        return;
    }

    updateState(beforeState_);
}

void PasteCommand::captureBeforeState()
{
    if (!layer_) {
        return;
    }

    int clippedX = std::max(0, regionX_);
    int clippedY = std::max(0, regionY_);
    int clippedWidth = std::min(regionWidth_, layer_->width() - clippedX);
    int clippedHeight = std::min(regionHeight_, layer_->height() - clippedY);

    if (clippedWidth <= 0 || clippedHeight <= 0) {
        beforeState_.clear();
        return;
    }

    beforeState_.resize(static_cast<std::size_t>(clippedWidth * clippedHeight) * 4U);

    const auto& layerData = layer_->data();
    const int layerWidth = layer_->width();
    const int pixelSize = 4;

    for (int row = 0; row < clippedHeight; ++row) {
        const int srcRow = clippedY + row;
        const int srcOffset = (srcRow * layerWidth + clippedX) * pixelSize;
        const int dstOffset = row * clippedWidth * pixelSize;

        const std::size_t rowBytes =
            static_cast<std::size_t>(clippedWidth) * static_cast<std::size_t>(pixelSize);
        std::memcpy(beforeState_.data() + dstOffset, layerData.data() + srcOffset, rowBytes);
    }
}

void PasteCommand::captureAfterState()
{
    if (!layer_) {
        return;
    }

    int clippedX = std::max(0, regionX_);
    int clippedY = std::max(0, regionY_);
    int clippedWidth = std::min(regionWidth_, layer_->width() - clippedX);
    int clippedHeight = std::min(regionHeight_, layer_->height() - clippedY);

    if (clippedWidth <= 0 || clippedHeight <= 0) {
        afterState_.clear();
        return;
    }

    afterState_.resize(static_cast<std::size_t>(clippedWidth * clippedHeight) * 4U);

    const auto& layerData = layer_->data();
    const int layerWidth = layer_->width();
    const int pixelSize = 4;

    for (int row = 0; row < clippedHeight; ++row) {
        const int srcRow = clippedY + row;
        const int srcOffset = (srcRow * layerWidth + clippedX) * pixelSize;
        const int dstOffset = row * clippedWidth * pixelSize;

        const std::size_t rowBytes =
            static_cast<std::size_t>(clippedWidth) * static_cast<std::size_t>(pixelSize);
        std::memcpy(afterState_.data() + dstOffset, layerData.data() + srcOffset, rowBytes);
    }
}

void PasteCommand::updateState(const std::vector<std::uint8_t>& state)
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
    const int pixelSize = 4;

    for (int row = 0; row < clippedHeight; ++row) {
        const int dstRow = clippedY + row;
        const int dstOffset = (dstRow * layerWidth + clippedX) * pixelSize;
        const int srcOffset = row * clippedWidth * pixelSize;

        const std::size_t rowBytes =
            static_cast<std::size_t>(clippedWidth) * static_cast<std::size_t>(pixelSize);
        std::memcpy(layerData.data() + dstOffset, state.data() + srcOffset, rowBytes);
    }
}

void PasteCommand::writeImageToLayer()
{
    if (!layer_ || imageData_.empty()) {
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
    const int pixelSize = 4;

    for (int row = 0; row < clippedHeight; ++row) {
        const int dstRow = clippedY + row;
        const int dstOffset = (dstRow * layerWidth + clippedX) * pixelSize;
        const int srcOffset = row * regionWidth_ * pixelSize;

        const std::size_t rowBytes =
            static_cast<std::size_t>(clippedWidth) * static_cast<std::size_t>(pixelSize);
        std::memcpy(layerData.data() + dstOffset, imageData_.data() + srcOffset, rowBytes);
    }
}

}  // namespace gimp
