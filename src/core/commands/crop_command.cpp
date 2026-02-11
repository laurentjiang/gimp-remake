/**
 * @file crop_command.cpp
 * @brief Command to crop document canvas.
 * @author Aless Tosi
 * @date 2026-02-09
 */

#include "core/commands/crop_command.h"

#include "core/document.h"
#include "core/event_bus.h"
#include "core/events.h"
#include "core/layer.h"

#include <QTransform>

#include <algorithm>
#include <cmath>

namespace gimp {

CropCommand::CropCommand(std::shared_ptr<Document> document, QRect cropBounds)
    : document_{std::move(document)},
      cropBounds_{cropBounds}
{
    targetWidth_ = cropBounds_.width();
    targetHeight_ = cropBounds_.height();
    valid_ = targetWidth_ > 0 && targetHeight_ > 0;
}

void CropCommand::apply()
{
    if (!document_ || !valid_) {
        return;
    }

    if (!captured_) {
        captureBeforeState();
    }

    document_->resize(targetWidth_, targetHeight_, anchorX_, anchorY_);
    restoreSelection(afterSelection_, afterSelectionType_, "crop");
}

void CropCommand::undo()
{
    if (!document_ || !captured_) {
        return;
    }

    document_->resize(beforeWidth_, beforeHeight_, 0.0F, 0.0F);
    restoreBeforeLayers();
    restoreSelection(beforeSelection_, beforeSelectionType_, "undo");
}

void CropCommand::captureBeforeState()
{
    if (!document_) {
        return;
    }

    beforeWidth_ = document_->width();
    beforeHeight_ = document_->height();

    beforeLayers_.clear();
    beforeLayers_.reserve(document_->layers().count());
    for (size_t i = 0; i < document_->layers().count(); ++i) {
        auto layer = document_->layers()[i];
        if (!layer) {
            continue;
        }

        LayerSnapshot snapshot;
        snapshot.layer = layer;
        snapshot.data = layer->data();
        beforeLayers_.push_back(std::move(snapshot));
    }

    computeAnchor();
    beforeSelection_ = SelectionManager::instance().selectionPath();
    beforeSelectionType_ = SelectionManager::instance().selectionType();
    afterSelection_ = buildAfterSelection();
    afterSelectionType_ = beforeSelectionType_;
    captured_ = true;
}

void CropCommand::restoreBeforeLayers()
{
    for (auto& snapshot : beforeLayers_) {
        if (!snapshot.layer) {
            continue;
        }

        snapshot.layer->data() = snapshot.data;
    }
}

void CropCommand::restoreSelection(const QPainterPath& path, SelectionType type, const char* source)
{
    SelectionManager::instance().restoreSelection(path, type);
    const bool hasSelection = !path.isEmpty();
    // NOLINTNEXTLINE(modernize-use-designated-initializers)
    EventBus::instance().publish(SelectionChangedEvent{hasSelection, source});
}

QPainterPath CropCommand::buildAfterSelection() const
{
    // Crop to selection: the canvas now matches the selection bounds exactly
    // No selection should remain after crop
    return QPainterPath();
}

QPoint CropCommand::computeOffset() const
{
    const float clampedX = std::clamp(anchorX_, 0.0F, 1.0F);
    const float clampedY = std::clamp(anchorY_, 0.0F, 1.0F);
    const float deltaX = static_cast<float>(targetWidth_ - beforeWidth_) * clampedX;
    const float deltaY = static_cast<float>(targetHeight_ - beforeHeight_) * clampedY;
    return {static_cast<int>(std::round(deltaX)), static_cast<int>(std::round(deltaY))};
}

void CropCommand::computeAnchor()
{
    if (beforeWidth_ == targetWidth_) {
        anchorX_ = 0.0F;
    } else {
        anchorX_ = static_cast<float>(cropBounds_.left()) /
                   static_cast<float>(beforeWidth_ - targetWidth_);
    }

    if (beforeHeight_ == targetHeight_) {
        anchorY_ = 0.0F;
    } else {
        anchorY_ = static_cast<float>(cropBounds_.top()) /
                   static_cast<float>(beforeHeight_ - targetHeight_);
    }
}

}  // namespace gimp
