/**
 * @file resize_command.cpp
 * @brief Command to resize document canvas.
 * @author Aless Tosi
 * @date 2026-02-09
 */

#include "core/commands/resize_command.h"

#include "core/document.h"
#include "core/event_bus.h"
#include "core/events.h"
#include "core/layer.h"

#include <QTransform>

#include <algorithm>
#include <cmath>

namespace gimp {

CanvasResizeCommand::CanvasResizeCommand(std::shared_ptr<Document> document,
                                         int width,
                                         int height,
                                         float anchorX,
                                         float anchorY)
    : document_{std::move(document)},
      targetWidth_{width},
      targetHeight_{height},
      anchorX_{anchorX},
      anchorY_{anchorY}
{
}

void CanvasResizeCommand::apply()
{
    if (!document_) {
        return;
    }

    if (!captured_) {
        captureBeforeState();
    }

    document_->resize(targetWidth_, targetHeight_, anchorX_, anchorY_);
    restoreSelection(afterSelection_, afterSelectionType_, "resize");
}

void CanvasResizeCommand::undo()
{
    if (!document_ || !captured_) {
        return;
    }

    document_->resize(beforeWidth_, beforeHeight_, 0.0F, 0.0F);
    restoreBeforeLayers();
    restoreSelection(beforeSelection_, beforeSelectionType_, "undo");
}

void CanvasResizeCommand::captureBeforeState()
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

    beforeSelection_ = SelectionManager::instance().selectionPath();
    beforeSelectionType_ = SelectionManager::instance().selectionType();
    afterSelection_ = buildAfterSelection();
    afterSelectionType_ = beforeSelectionType_;
    captured_ = true;
}

void CanvasResizeCommand::restoreBeforeLayers()
{
    for (auto& snapshot : beforeLayers_) {
        if (!snapshot.layer) {
            continue;
        }

        snapshot.layer->data() = snapshot.data;
    }
}

void CanvasResizeCommand::restoreSelection(const QPainterPath& path,
                                           SelectionType type,
                                           const char* source)
{
    SelectionManager::instance().restoreSelection(path, type);
    const bool hasSelection = !path.isEmpty();
    // NOLINTNEXTLINE(modernize-use-designated-initializers)
    EventBus::instance().publish(SelectionChangedEvent{hasSelection, source});
}

QPainterPath CanvasResizeCommand::buildAfterSelection() const
{
    if (beforeSelection_.isEmpty()) {
        return QPainterPath();
    }

    const QPoint offset = computeOffset();
    QTransform transform;
    transform.translate(offset.x(), offset.y());
    QPainterPath moved = transform.map(beforeSelection_);

    QPainterPath bounds;
    bounds.addRect(0, 0, targetWidth_, targetHeight_);
    return moved.intersected(bounds);
}

QPoint CanvasResizeCommand::computeOffset() const
{
    const float clampedX = std::clamp(anchorX_, 0.0F, 1.0F);
    const float clampedY = std::clamp(anchorY_, 0.0F, 1.0F);
    const float deltaX = static_cast<float>(targetWidth_ - beforeWidth_) * clampedX;
    const float deltaY = static_cast<float>(targetHeight_ - beforeHeight_) * clampedY;
    return {static_cast<int>(std::round(deltaX)), static_cast<int>(std::round(deltaY))};
}

}  // namespace gimp
