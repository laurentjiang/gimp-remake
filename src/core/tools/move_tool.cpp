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

#include <QImage>
#include <QPainter>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace gimp {

namespace {
/// Handle size in screen pixels for transform handles.
constexpr int kHandleSize = 8;
/// Half the handle size, used for centering handles on corners/edges.
constexpr int kHandleHalfSize = kHandleSize / 2;
}  // namespace

void MoveTool::beginStroke(const ToolInputEvent& event)
{
    proportionalScale_ = (event.modifiers & Qt::ShiftModifier) != 0;

    // If we already have an active floating buffer, check for handle hits first
    if (isMovingSelection()) {
        activeHandle_ = hitTestHandle(event.canvasPos, event.zoomLevel);
        if (activeHandle_ != TransformHandle::None) {
            // Capture current offset BEFORE overwriting startPos_
            QPoint currentOffset = floatingOffset();

            // Starting a scale operation - store anchor point (opposite corner)
            // Set startPos_ to preserve the offset so floatingOffset() returns currentOffset
            currentPos_ = event.canvasPos;
            startPos_ = currentPos_ - currentOffset;
            originalSize_ = QSizeF(floatingRect_.width() * currentScale_.width(),
                                   floatingRect_.height() * currentScale_.height());

            // Calculate current transformed bounds for anchor using captured offset
            QRectF currentBounds(floatingRect_.x() + currentOffset.x(),
                                 floatingRect_.y() + currentOffset.y(),
                                 floatingRect_.width() * currentScale_.width(),
                                 floatingRect_.height() * currentScale_.height());
            scaleAnchor_ = getAnchorForHandle(activeHandle_, currentBounds);
            return;
        }

        // Not on a handle - continue move
        QPoint currentOffset = floatingOffset();
        startPos_ = event.canvasPos - currentOffset;
        currentPos_ = event.canvasPos;
        return;
    }

    startPos_ = event.canvasPos;
    currentPos_ = event.canvasPos;
    clearFloatingState();

    // Check if we have a selection and click is inside it
    auto& selMgr = SelectionManager::instance();
    if (!document_ || document_->layers().count() == 0) {
        return;
    }

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

    // Determine effective copy mode: modifier override takes precedence over UI setting
    bool effectiveCopyMode = modifierOverride_ ? modifierCopyMode_ : (moveMode_ == MoveMode::Copy);

    // Only clear source if not in copy mode (Shift+Alt = copy, Ctrl+Alt = cut)
    if (!effectiveCopyMode) {
        clearSourcePixels(layer);
    }
}

void MoveTool::continueStroke(const ToolInputEvent& event)
{
    proportionalScale_ = (event.modifiers & Qt::ShiftModifier) != 0;
    currentPos_ = event.canvasPos;

    // If scaling, update scale factors based on anchor and mouse position
    if (activeHandle_ != TransformHandle::None && !floatingRect_.isEmpty()) {
        double origW = static_cast<double>(floatingRect_.width());
        double origH = static_cast<double>(floatingRect_.height());
        double mouseX = static_cast<double>(currentPos_.x());
        double mouseY = static_cast<double>(currentPos_.y());

        // Calculate new size based on distance from anchor to mouse
        double newScaleX = currentScale_.width();
        double newScaleY = currentScale_.height();

        switch (activeHandle_) {
            case TransformHandle::TopLeft:
                // Mouse is top-left, anchor is bottom-right
                newScaleX = (scaleAnchor_.x() - mouseX) / origW;
                newScaleY = (scaleAnchor_.y() - mouseY) / origH;
                break;
            case TransformHandle::TopRight:
                // Mouse is top-right, anchor is bottom-left
                newScaleX = (mouseX - scaleAnchor_.x()) / origW;
                newScaleY = (scaleAnchor_.y() - mouseY) / origH;
                break;
            case TransformHandle::BottomLeft:
                // Mouse is bottom-left, anchor is top-right
                newScaleX = (scaleAnchor_.x() - mouseX) / origW;
                newScaleY = (mouseY - scaleAnchor_.y()) / origH;
                break;
            case TransformHandle::BottomRight:
                // Mouse is bottom-right, anchor is top-left
                newScaleX = (mouseX - scaleAnchor_.x()) / origW;
                newScaleY = (mouseY - scaleAnchor_.y()) / origH;
                break;
            case TransformHandle::Top:
                // Mouse defines top edge, anchor is bottom
                newScaleY = (scaleAnchor_.y() - mouseY) / origH;
                break;
            case TransformHandle::Bottom:
                // Mouse defines bottom edge, anchor is top
                newScaleY = (mouseY - scaleAnchor_.y()) / origH;
                break;
            case TransformHandle::Left:
                // Mouse defines left edge, anchor is right
                newScaleX = (scaleAnchor_.x() - mouseX) / origW;
                break;
            case TransformHandle::Right:
                // Mouse defines right edge, anchor is left
                newScaleX = (mouseX - scaleAnchor_.x()) / origW;
                break;
            case TransformHandle::None:
                break;
        }

        // Clamp to minimum size (10% of original)
        newScaleX = std::max(0.1, newScaleX);
        newScaleY = std::max(0.1, newScaleY);

        // Apply proportional constraint if Shift is held
        if (proportionalScale_) {
            double avgScale = (newScaleX + newScaleY) / 2.0;
            // For corner handles, use the larger scale
            if (activeHandle_ == TransformHandle::TopLeft ||
                activeHandle_ == TransformHandle::TopRight ||
                activeHandle_ == TransformHandle::BottomLeft ||
                activeHandle_ == TransformHandle::BottomRight) {
                avgScale = std::max(newScaleX, newScaleY);
            }
            newScaleX = avgScale;
            newScaleY = avgScale;
        }

        currentScale_ = QSizeF(newScaleX, newScaleY);

        // Update position offset to keep anchor fixed
        // New top-left = anchor - (scaled size in anchor's direction)
        double scaledW = origW * newScaleX;
        double scaledH = origH * newScaleY;
        double newLeft = 0;
        double newTop = 0;

        switch (activeHandle_) {
            case TransformHandle::TopLeft:
            case TransformHandle::Top:
            case TransformHandle::Left:
                // Anchor is on right/bottom side, new position moves
                newLeft = scaleAnchor_.x() - scaledW;
                newTop = scaleAnchor_.y() - scaledH;
                if (activeHandle_ == TransformHandle::Top) {
                    newLeft = floatingRect_.x();  // X doesn't change for top edge
                }
                if (activeHandle_ == TransformHandle::Left) {
                    newTop = floatingRect_.y();  // Y doesn't change for left edge
                }
                break;
            case TransformHandle::TopRight:
                newLeft = scaleAnchor_.x();
                newTop = scaleAnchor_.y() - scaledH;
                break;
            case TransformHandle::BottomLeft:
                newLeft = scaleAnchor_.x() - scaledW;
                newTop = scaleAnchor_.y();
                break;
            case TransformHandle::BottomRight:
            case TransformHandle::Bottom:
            case TransformHandle::Right:
                // Anchor is on left/top side, position comes from anchor
                newLeft = scaleAnchor_.x();
                newTop = scaleAnchor_.y();
                if (activeHandle_ == TransformHandle::Bottom) {
                    newLeft = floatingRect_.x();  // X doesn't change
                }
                if (activeHandle_ == TransformHandle::Right) {
                    newTop = floatingRect_.y();  // Y doesn't change
                }
                break;
            case TransformHandle::None:
                newLeft = floatingRect_.x();
                newTop = floatingRect_.y();
                break;
        }

        // Update startPos_ so floatingOffset() returns the correct offset
        // floatingOffset = currentPos_ - startPos_ should equal (newLeft - floatingRect_.x(),
        // newTop - floatingRect_.y())
        int offsetX = static_cast<int>(std::round(newLeft)) - floatingRect_.x();
        int offsetY = static_cast<int>(std::round(newTop)) - floatingRect_.y();
        startPos_ = currentPos_ - QPoint(offsetX, offsetY);
    }
}

bool MoveTool::isDestinationInsideBounds() const
{
    if (!targetLayer_ || floatingRect_.isEmpty()) {
        return true;
    }
    QPoint offset = currentPos_ - startPos_;
    QRect dstRect = floatingRect_.translated(offset);
    QRect layerBounds(0, 0, targetLayer_->width(), targetLayer_->height());
    return layerBounds.contains(dstRect);
}

void MoveTool::endStroke(const ToolInputEvent& event)
{
    currentPos_ = event.canvasPos;
    lastDelta_ = currentPos_ - startPos_;

    // Keep floating buffer active after mouse release so user can:
    // - See and drag transform handles
    // - Continue adjusting position
    // - Press Enter to commit or Escape to cancel
    // This matches GIMP transform tool behavior.
    if (activeHandle_ != TransformHandle::None) {
        activeHandle_ = TransformHandle::None;
    }
    // Don't auto-commit - require explicit Enter key to finalize the transform
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

    spdlog::debug("[MoveTool] Extracting selection pixels, bounds: ({},{}) {}x{}",
                  bounding.x(),
                  bounding.y(),
                  bounding.width(),
                  bounding.height());

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
        modifierOverride_ = false;
        return;
    }

    // Determine effective copy mode: modifier override takes precedence over UI setting
    bool effectiveCopyMode = modifierOverride_ ? modifierCopyMode_ : (moveMode_ == MoveMode::Copy);

    QPoint offset = floatingOffset();
    bool hasScale = std::abs(currentScale_.width() - 1.0) > 0.001 ||
                    std::abs(currentScale_.height() - 1.0) > 0.001;

    spdlog::debug("[MoveTool] Committing move: offset=({},{}) scale=({:.2f},{:.2f}) copy={}",
                  offset.x(),
                  offset.y(),
                  currentScale_.width(),
                  currentScale_.height(),
                  effectiveCopyMode);

    // Calculate the bounding rect that covers both source and destination
    QRect srcRect = floatingRect_;
    QSize scaledSize = getScaledSize();
    QRect dstRect(floatingRect_.x() + offset.x(),
                  floatingRect_.y() + offset.y(),
                  scaledSize.width(),
                  scaledSize.height());
    QRect unionRect = srcRect.united(dstRect);

    // Clip to layer bounds
    unionRect = unionRect.intersected(QRect(0, 0, targetLayer_->width(), targetLayer_->height()));

    if (unionRect.isEmpty()) {
        clearFloatingState();
        modifierOverride_ = false;
        return;
    }

    // Create command and capture before state
    auto cmd = std::make_shared<MoveCommand>(targetLayer_, unionRect);

    // Get scaled buffer if needed
    std::vector<std::uint8_t> scaledBuf;
    if (hasScale) {
        scaledBuf = getScaledBuffer();
    }

    auto pasteBuffer = [&](QPoint pasteOffset, bool scaled) {
        auto& layerData = targetLayer_->data();
        int layerWidth = targetLayer_->width();
        int layerHeight = targetLayer_->height();
        constexpr int kPixelSize = 4;

        const std::vector<std::uint8_t>& srcBuf = scaled ? scaledBuf : floatingBuffer_;
        int srcW = scaled ? scaledSize.width() : floatingRect_.width();
        int srcH = scaled ? scaledSize.height() : floatingRect_.height();

        int dstX = floatingRect_.x() + pasteOffset.x();
        int dstY = floatingRect_.y() + pasteOffset.y();

        for (int row = 0; row < srcH; ++row) {
            for (int col = 0; col < srcW; ++col) {
                int destPx = dstX + col;
                int destPy = dstY + row;

                if (destPx < 0 || destPx >= layerWidth || destPy < 0 || destPy >= layerHeight) {
                    continue;
                }

                std::size_t srcOffset = (static_cast<std::size_t>(row) * srcW + col) * kPixelSize;
                std::size_t dstOffset =
                    (static_cast<std::size_t>(destPy) * layerWidth + destPx) * kPixelSize;

                // Only paste non-transparent pixels (check alpha)
                if (srcBuf[srcOffset + 3] > 0) {
                    std::memcpy(
                        layerData.data() + dstOffset, srcBuf.data() + srcOffset, kPixelSize);
                }
            }
        }
    };

    if (effectiveCopyMode) {
        // Copy mode: source was never cleared, just paste at new location
        cmd->captureBeforeState();
        pasteBuffer(offset, hasScale);
        cmd->captureAfterState();
    } else {
        // Cut mode: source was cleared, need to restore for before state
        // Restore original pixels temporarily to capture before state
        pasteFloatingBuffer(targetLayer_, QPoint(0, 0));  // Paste at original location
        cmd->captureBeforeState();

        // Clear again and paste at new location
        clearSourcePixels(targetLayer_);
        pasteBuffer(offset, hasScale);
        cmd->captureAfterState();
    }

    // Dispatch command
    if (commandBus_) {
        commandBus_->dispatch(cmd);
    }

    // Update selection outline to follow the moved/scaled pixels
    // For scaling, we should scale the selection path too
    if (hasScale) {
        // Scale and translate selection
        SelectionManager::instance().scaleSelection(currentScale_, offset);
    } else {
        SelectionManager::instance().translateSelection(offset);
    }

    clearFloatingState();
    modifierOverride_ = false;
}

void MoveTool::cancelMove()
{
    spdlog::debug("[MoveTool] Cancelling move, restoring original pixels");
    if (!targetLayer_ || floatingBuffer_.empty()) {
        clearFloatingState();
        modifierOverride_ = false;
        return;
    }

    // Determine effective copy mode: modifier override takes precedence over UI setting
    bool effectiveCopyMode = modifierOverride_ ? modifierCopyMode_ : (moveMode_ == MoveMode::Copy);

    // Only restore if we were in cut mode (source was cleared)
    if (!effectiveCopyMode) {
        pasteFloatingBuffer(targetLayer_, QPoint(0, 0));
    }

    clearFloatingState();
    modifierOverride_ = false;
}

void MoveTool::commitFloatingBuffer()
{
    if (!floatingBuffer_.empty()) {
        commitMove();
    }
}

void MoveTool::cancelFloatingBuffer()
{
    if (!floatingBuffer_.empty()) {
        cancelMove();
        state_ = ToolState::Idle;
    }
}

void MoveTool::clearFloatingState()
{
    floatingBuffer_.clear();
    floatingRect_ = QRect();
    targetLayer_.reset();
    selectionMask_.clear();
    activeHandle_ = TransformHandle::None;
    currentScale_ = QSizeF(1.0, 1.0);
    originalSize_ = QSizeF();
    scaleAnchor_ = QPointF();
    proportionalScale_ = false;
}

void MoveTool::rasterizeSelectionMask(const QPainterPath& selPath, const QRect& bounds)
{
    int width = bounds.width();
    int height = bounds.height();
    int x1 = bounds.left();
    int y1 = bounds.top();

    selectionMask_.resize(static_cast<std::size_t>(width) * height, false);

    // Use selection type hint for optimized rasterization
    SelectionType selType = SelectionManager::instance().selectionType();
    QRectF pathBounds = selPath.boundingRect();

    if (selType == SelectionType::Rectangle) {
        // Rectangle: Direct bounds check (O(1) per pixel)
        int rectX1 = static_cast<int>(std::floor(pathBounds.left()));
        int rectY1 = static_cast<int>(std::floor(pathBounds.top()));
        int rectX2 = static_cast<int>(std::ceil(pathBounds.right()));
        int rectY2 = static_cast<int>(std::ceil(pathBounds.bottom()));

        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                int px = x1 + col;
                int py = y1 + row;
                selectionMask_[static_cast<std::size_t>(row) * width + col] =
                    (px >= rectX1 && px < rectX2 && py >= rectY1 && py < rectY2);
            }
        }
    } else if (selType == SelectionType::Ellipse) {
        // Ellipse: Direct equation check (O(1) per pixel)
        double cx = pathBounds.center().x();
        double cy = pathBounds.center().y();
        double rx = pathBounds.width() / 2.0;
        double ry = pathBounds.height() / 2.0;

        // Avoid division by zero
        if (rx < 0.5 || ry < 0.5) {
            return;
        }

        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                double px = x1 + col + 0.5;
                double py = y1 + row + 0.5;
                double dx = (px - cx) / rx;
                double dy = (py - cy) / ry;
                selectionMask_[static_cast<std::size_t>(row) * width + col] =
                    (dx * dx + dy * dy) <= 1.0;
            }
        }
    } else {
        // Unknown/freeform: Use QPainter rasterization for efficient mask generation
        // This is O(height * path_complexity) instead of O(width * height * path_edges)
        QImage maskImage(width, height, QImage::Format_Grayscale8);
        maskImage.fill(0);

        QPainter painter(&maskImage);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(Qt::NoPen);
        painter.setBrush(Qt::white);
        painter.translate(-x1, -y1);  // Translate to local coordinates
        painter.drawPath(selPath);
        painter.end();

        // Extract boolean mask from grayscale scanlines
        for (int row = 0; row < height; ++row) {
            const uchar* scanline = maskImage.constScanLine(row);
            for (int col = 0; col < width; ++col) {
                selectionMask_[static_cast<std::size_t>(row) * width + col] = (scanline[col] > 0);
            }
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

std::vector<ToolOption> MoveTool::getOptions() const
{
    std::vector<ToolOption> options;

    ToolOption modeOption;
    modeOption.id = "move_mode";
    modeOption.label = "Mode";
    modeOption.type = ToolOption::Type::Dropdown;
    modeOption.choices = {"Cut", "Copy"};
    modeOption.selectedIndex = (moveMode_ == MoveMode::Cut) ? 0 : 1;
    modeOption.value = modeOption.selectedIndex;
    options.push_back(modeOption);

    return options;
}

void MoveTool::setOptionValue(const std::string& optionId,
                              const std::variant<int, float, bool, std::string>& value)
{
    if (optionId == "move_mode") {
        if (const auto* strVal = std::get_if<std::string>(&value)) {
            moveMode_ = (*strVal == "Copy") ? MoveMode::Copy : MoveMode::Cut;
        } else if (const auto* intVal = std::get_if<int>(&value)) {
            moveMode_ = (*intVal == 1) ? MoveMode::Copy : MoveMode::Cut;
        }
    }
}

std::variant<int, float, bool, std::string> MoveTool::getOptionValue(
    const std::string& optionId) const
{
    if (optionId == "move_mode") {
        return (moveMode_ == MoveMode::Cut) ? 0 : 1;
    }
    return 0;
}

std::vector<QRect> MoveTool::getHandleRects(float zoomLevel) const
{
    std::vector<QRect> handles;
    handles.reserve(8);
    if (floatingRect_.isEmpty()) {
        return handles;
    }

    // Handle size compensated for zoom (8 screen pixels)
    constexpr float kHandleScreenSize = 8.0F;
    int handleSize = static_cast<int>(std::ceil(kHandleScreenSize / zoomLevel));
    int halfSize = handleSize / 2;

    // Calculate the transformed bounding box
    QPoint offset = floatingOffset();
    int scaledW = static_cast<int>(floatingRect_.width() * currentScale_.width());
    int scaledH = static_cast<int>(floatingRect_.height() * currentScale_.height());

    int left = floatingRect_.x() + offset.x();
    int top = floatingRect_.y() + offset.y();
    int right = left + scaledW;
    int bottom = top + scaledH;
    int midX = (left + right) / 2;
    int midY = (top + bottom) / 2;

    // Order: TopLeft, Top, TopRight, Right, BottomRight, Bottom, BottomLeft, Left
    handles.push_back(QRect(left - halfSize, top - halfSize, handleSize, handleSize));
    handles.push_back(QRect(midX - halfSize, top - halfSize, handleSize, handleSize));
    handles.push_back(QRect(right - halfSize, top - halfSize, handleSize, handleSize));
    handles.push_back(QRect(right - halfSize, midY - halfSize, handleSize, handleSize));
    handles.push_back(QRect(right - halfSize, bottom - halfSize, handleSize, handleSize));
    handles.push_back(QRect(midX - halfSize, bottom - halfSize, handleSize, handleSize));
    handles.push_back(QRect(left - halfSize, bottom - halfSize, handleSize, handleSize));
    handles.push_back(QRect(left - halfSize, midY - halfSize, handleSize, handleSize));

    return handles;
}

TransformHandle MoveTool::hitTestHandle(const QPoint& pos, float zoomLevel) const
{
    auto handles = getHandleRects(zoomLevel);
    if (handles.empty()) {
        return TransformHandle::None;
    }

    // Check each handle (order matches TransformHandle enum starting from TopLeft)
    static const TransformHandle handleTypes[] = {TransformHandle::TopLeft,
                                                  TransformHandle::Top,
                                                  TransformHandle::TopRight,
                                                  TransformHandle::Right,
                                                  TransformHandle::BottomRight,
                                                  TransformHandle::Bottom,
                                                  TransformHandle::BottomLeft,
                                                  TransformHandle::Left};

    for (size_t i = 0; i < handles.size(); ++i) {
        if (handles[i].contains(pos)) {
            return handleTypes[i];
        }
    }

    return TransformHandle::None;
}

QSize MoveTool::getScaledSize() const
{
    if (floatingRect_.isEmpty()) {
        return QSize();
    }
    int w = static_cast<int>(std::round(floatingRect_.width() * currentScale_.width()));
    int h = static_cast<int>(std::round(floatingRect_.height() * currentScale_.height()));
    return QSize(std::max(1, w), std::max(1, h));
}

std::vector<std::uint8_t> MoveTool::getScaledBuffer() const
{
    if (floatingBuffer_.empty() || floatingRect_.isEmpty()) {
        return {};
    }

    // If scale is 1:1, return the original buffer
    if (std::abs(currentScale_.width() - 1.0) < 0.001 &&
        std::abs(currentScale_.height() - 1.0) < 0.001) {
        return floatingBuffer_;
    }

    QSize scaledSize = getScaledSize();
    int srcW = floatingRect_.width();
    int srcH = floatingRect_.height();
    int dstW = scaledSize.width();
    int dstH = scaledSize.height();

    std::vector<std::uint8_t> scaled(static_cast<size_t>(dstW * dstH) * 4, 0);

    // Bilinear interpolation for scaling
    for (int dstY = 0; dstY < dstH; ++dstY) {
        for (int dstX = 0; dstX < dstW; ++dstX) {
            // Map destination pixel to source coordinates
            double srcXf = (static_cast<double>(dstX) + 0.5) / currentScale_.width() - 0.5;
            double srcYf = (static_cast<double>(dstY) + 0.5) / currentScale_.height() - 0.5;

            int srcX0 = static_cast<int>(std::floor(srcXf));
            int srcY0 = static_cast<int>(std::floor(srcYf));
            int srcX1 = srcX0 + 1;
            int srcY1 = srcY0 + 1;

            // Clamp to bounds
            srcX0 = std::clamp(srcX0, 0, srcW - 1);
            srcY0 = std::clamp(srcY0, 0, srcH - 1);
            srcX1 = std::clamp(srcX1, 0, srcW - 1);
            srcY1 = std::clamp(srcY1, 0, srcH - 1);

            double fx = srcXf - std::floor(srcXf);
            double fy = srcYf - std::floor(srcYf);

            // Get 4 source pixels
            auto getPixel = [&](int x, int y, int c) -> int {
                return floatingBuffer_[(static_cast<size_t>(y) * srcW + x) * 4 + c];
            };

            // Bilinear interpolation for each channel
            size_t dstIdx = (static_cast<size_t>(dstY) * dstW + dstX) * 4;
            for (int c = 0; c < 4; ++c) {
                double v00 = getPixel(srcX0, srcY0, c);
                double v10 = getPixel(srcX1, srcY0, c);
                double v01 = getPixel(srcX0, srcY1, c);
                double v11 = getPixel(srcX1, srcY1, c);

                double v0 = v00 * (1.0 - fx) + v10 * fx;
                double v1 = v01 * (1.0 - fx) + v11 * fx;
                double v = v0 * (1.0 - fy) + v1 * fy;

                scaled[dstIdx + c] = static_cast<std::uint8_t>(std::clamp(v, 0.0, 255.0));
            }
        }
    }

    return scaled;
}

QPointF MoveTool::getAnchorForHandle(TransformHandle handle, const QRectF& bounds)
{
    switch (handle) {
        case TransformHandle::TopLeft:
            return bounds.bottomRight();
        case TransformHandle::Top:
            return QPointF(bounds.center().x(), bounds.bottom());
        case TransformHandle::TopRight:
            return bounds.bottomLeft();
        case TransformHandle::Right:
            return QPointF(bounds.left(), bounds.center().y());
        case TransformHandle::BottomRight:
            return bounds.topLeft();
        case TransformHandle::Bottom:
            return QPointF(bounds.center().x(), bounds.top());
        case TransformHandle::BottomLeft:
            return bounds.topRight();
        case TransformHandle::Left:
            return QPointF(bounds.right(), bounds.center().y());
        case TransformHandle::None:
        default:
            return bounds.center();
    }
}

}  // namespace gimp
