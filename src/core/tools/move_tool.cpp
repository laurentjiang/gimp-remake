/**
 * @file move_tool.cpp
 * @brief Implementation of MoveTool.
 * @author Laurent Jiang
 * @date 2026-01-26
 */

#include "core/tools/move_tool.h"

#include "core/command_bus.h"
#include "core/commands/move_command.h"
#include "core/layer.h"
#include "core/selection_manager.h"

#include <algorithm>
#include <cstring>

namespace gimp {

void MoveTool::beginStroke(const ToolInputEvent& event)
{
    startPos_ = event.canvasPos;
    currentPos_ = event.canvasPos;
    clearFloatingState();

    // Check if we have a selection and click is inside it
    if (!document_ || document_->layers().count() == 0) {
        return;
    }

    auto& selMgr = SelectionManager::instance();
    if (!selMgr.hasSelection()) {
        // No selection - future: move entire layer
        return;
    }

    const QPainterPath& selPath = selMgr.selectionPath();
    if (!selPath.contains(QPointF(event.canvasPos))) {
        // Click outside selection - do nothing
        return;
    }

    // Extract pixels from selection
    auto layer = document_->layers()[0];
    targetLayer_ = layer;
    extractSelectionPixels(layer);

    // Only clear source if not in copy mode (Shift+Alt = copy, Ctrl+Alt = cut)
    if (!copyMode_) {
        clearSourcePixels(layer);
    }
}

void MoveTool::continueStroke(const ToolInputEvent& event)
{
    currentPos_ = event.canvasPos;
}

void MoveTool::endStroke(const ToolInputEvent& event)
{
    currentPos_ = event.canvasPos;
    lastDelta_ = currentPos_ - startPos_;

    if (isMovingSelection()) {
        commitMove();
    }
}

void MoveTool::cancelStroke()
{
    if (isMovingSelection()) {
        cancelMove();
    }
    lastDelta_ = QPoint(0, 0);
}

bool MoveTool::onKeyPress(Qt::Key key, Qt::KeyboardModifiers /*modifiers*/)
{
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        if (isMovingSelection()) {
            lastDelta_ = currentPos_ - startPos_;
            commitMove();
            state_ = ToolState::Idle;
            return true;
        }
    } else if (key == Qt::Key_Escape) {
        if (isMovingSelection()) {
            cancelMove();
            state_ = ToolState::Idle;
            return true;
        }
    }
    return false;
}

void MoveTool::extractSelectionPixels(const std::shared_ptr<Layer>& layer)
{
    if (!layer) {
        return;
    }

    const QPainterPath& selPath = SelectionManager::instance().selectionPath();
    QRectF boundingF = selPath.boundingRect();
    QRect bounding = boundingF.toAlignedRect();

    // Clip to layer bounds
    int x1 = std::max(0, bounding.left());
    int y1 = std::max(0, bounding.top());
    int x2 = std::min(layer->width(), bounding.right() + 1);
    int y2 = std::min(layer->height(), bounding.bottom() + 1);

    if (x2 <= x1 || y2 <= y1) {
        return;
    }

    int width = x2 - x1;
    int height = y2 - y1;
    floatingRect_ = QRect(x1, y1, width, height);

    // Pre-rasterize the selection mask for O(1) lookups
    rasterizeSelectionMask(selPath, floatingRect_);

    // Allocate buffer (RGBA, 4 bytes per pixel) - initialize to transparent
    floatingBuffer_.resize(static_cast<std::size_t>(width * height) * 4, 0);

    const auto& layerData = layer->data();
    int layerWidth = layer->width();
    constexpr int kPixelSize = 4;

    // Copy pixels that are inside the selection (using pre-rasterized mask)
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            if (isPixelSelected(col, row)) {
                int px = x1 + col;
                int py = y1 + row;
                std::size_t srcOffset =
                    (static_cast<std::size_t>(py) * layerWidth + px) * kPixelSize;
                std::size_t dstOffset = (static_cast<std::size_t>(row) * width + col) * kPixelSize;

                std::memcpy(
                    floatingBuffer_.data() + dstOffset, layerData.data() + srcOffset, kPixelSize);
            }
        }
    }
}

void MoveTool::clearSourcePixels(const std::shared_ptr<Layer>& layer)
{
    if (!layer || floatingRect_.isEmpty()) {
        return;
    }

    auto& layerData = layer->data();
    int layerWidth = layer->width();
    constexpr int kPixelSize = 4;

    int x1 = floatingRect_.left();
    int y1 = floatingRect_.top();
    int width = floatingRect_.width();
    int height = floatingRect_.height();

    // Clear pixels inside selection to transparent (using pre-rasterized mask)
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            if (isPixelSelected(col, row)) {
                int px = x1 + col;
                int py = y1 + row;
                std::size_t offset = (static_cast<std::size_t>(py) * layerWidth + px) * kPixelSize;
                layerData[offset + 0] = 0;  // R
                layerData[offset + 1] = 0;  // G
                layerData[offset + 2] = 0;  // B
                layerData[offset + 3] = 0;  // A
            }
        }
    }
}

void MoveTool::pasteFloatingBuffer(const std::shared_ptr<Layer>& layer, QPoint offset)
{
    if (!layer || floatingBuffer_.empty()) {
        return;
    }

    auto& layerData = layer->data();
    int layerWidth = layer->width();
    int layerHeight = layer->height();
    constexpr int kPixelSize = 4;

    int srcX = floatingRect_.left();
    int srcY = floatingRect_.top();
    int width = floatingRect_.width();
    int height = floatingRect_.height();

    int dstX = srcX + offset.x();
    int dstY = srcY + offset.y();

    // Paste pixels that were inside the selection (using pre-rasterized mask)
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            // Only paste pixels that were inside original selection
            if (!isPixelSelected(col, row)) {
                continue;
            }

            int destPx = dstX + col;
            int destPy = dstY + row;

            // Clip to layer bounds
            if (destPx < 0 || destPx >= layerWidth || destPy < 0 || destPy >= layerHeight) {
                continue;
            }

            std::size_t srcOffset = (static_cast<std::size_t>(row) * width + col) * kPixelSize;
            std::size_t dstOffset =
                (static_cast<std::size_t>(destPy) * layerWidth + destPx) * kPixelSize;

            std::memcpy(
                layerData.data() + dstOffset, floatingBuffer_.data() + srcOffset, kPixelSize);
        }
    }
}

void MoveTool::commitMove()
{
    if (!targetLayer_ || floatingBuffer_.empty()) {
        clearFloatingState();
        copyMode_ = false;
        return;
    }

    QPoint offset = currentPos_ - startPos_;

    // Calculate the bounding rect that covers both source and destination
    QRect srcRect = floatingRect_;
    QRect dstRect = floatingRect_.translated(offset);
    QRect unionRect = srcRect.united(dstRect);

    // Clip to layer bounds
    unionRect = unionRect.intersected(QRect(0, 0, targetLayer_->width(), targetLayer_->height()));

    if (unionRect.isEmpty()) {
        clearFloatingState();
        copyMode_ = false;
        return;
    }

    // Create command and capture before state
    auto cmd = std::make_shared<MoveCommand>(targetLayer_, unionRect);

    if (copyMode_) {
        // Copy mode: source was never cleared, just paste at new location
        cmd->captureBeforeState();
        pasteFloatingBuffer(targetLayer_, offset);
        cmd->captureAfterState();
    } else {
        // Cut mode: source was cleared, need to restore for before state
        // Restore original pixels temporarily to capture before state
        pasteFloatingBuffer(targetLayer_, QPoint(0, 0));  // Paste at original location
        cmd->captureBeforeState();

        // Clear again and paste at new location
        clearSourcePixels(targetLayer_);
        pasteFloatingBuffer(targetLayer_, offset);
        cmd->captureAfterState();
    }

    // Dispatch command
    if (commandBus_) {
        commandBus_->dispatch(cmd);
    }

    clearFloatingState();
    copyMode_ = false;
}

void MoveTool::cancelMove()
{
    if (!targetLayer_ || floatingBuffer_.empty()) {
        clearFloatingState();
        copyMode_ = false;
        return;
    }

    // Only restore if we were in cut mode (source was cleared)
    if (!copyMode_) {
        pasteFloatingBuffer(targetLayer_, QPoint(0, 0));
    }

    clearFloatingState();
    copyMode_ = false;
}

void MoveTool::clearFloatingState()
{
    floatingBuffer_.clear();
    floatingRect_ = QRect();
    targetLayer_.reset();
    selectionMask_.clear();
}

void MoveTool::rasterizeSelectionMask(const QPainterPath& selPath, const QRect& bounds)
{
    int width = bounds.width();
    int height = bounds.height();
    int x1 = bounds.left();
    int y1 = bounds.top();

    selectionMask_.resize(static_cast<std::size_t>(width) * height, false);

    // Rasterize the selection path to a boolean mask (one-time O(n) operation)
    for (int row = 0; row < height; ++row) {
        for (int col = 0; col < width; ++col) {
            int px = x1 + col;
            int py = y1 + row;
            selectionMask_[static_cast<std::size_t>(row) * width + col] =
                selPath.contains(QPointF(px + 0.5, py + 0.5));
        }
    }
}

bool MoveTool::isPixelSelected(int col, int row) const
{
    if (selectionMask_.empty() || floatingRect_.isEmpty()) {
        return false;
    }
    int width = floatingRect_.width();
    if (col < 0 || col >= width || row < 0 || row >= floatingRect_.height()) {
        return false;
    }
    return selectionMask_[static_cast<std::size_t>(row) * width + col];
}

}  // namespace gimp
