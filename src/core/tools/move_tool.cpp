/**
 * @file move_tool.cpp
 * @brief Implementation of MoveTool using FloatingBuffer and TransformState.
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
constexpr float kHandleScreenSize = 8.0F;
}  // namespace

void MoveTool::beginStroke(const ToolInputEvent& event)
{
    proportionalScale_ = (event.modifiers & Qt::ShiftModifier) != 0;

    // If we already have an active floating buffer, check for handle hits first
    if (isMovingSelection()) {
        QRectF txBounds = transform_.transformedBounds();
        spdlog::debug("[MoveTool] Active buffer exists, transformed bounds: ({:.1f},{:.1f}) {:.1f}x{:.1f}",
                      txBounds.left(),
                      txBounds.top(),
                      txBounds.width(),
                      txBounds.height());

        qreal handleSize = kHandleScreenSize / event.zoomLevel;
        activeHandle_ = transform_.hitTestHandle(QPointF(event.canvasPos), handleSize);

        if (activeHandle_ != TransformHandle::None) {
            // Starting a scale operation - capture anchor at this moment
            transform_.beginHandleDrag(activeHandle_);
            startPos_ = event.canvasPos;
            currentPos_ = event.canvasPos;
            spdlog::debug("[MoveTool] Begin scale operation, handle: {}",
                          static_cast<int>(activeHandle_));
            return;
        }

        // Not on a handle - continue move by updating drag start
        spdlog::debug("[MoveTool] Continuing move (no handle hit), click at ({},{})",
                      event.canvasPos.x(),
                      event.canvasPos.y());
        startPos_ = event.canvasPos - transform_.translation().toPoint();
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

    // Extract pixels from selection using FloatingBuffer
    auto layer = document_->layers()[0];
    targetLayer_ = layer;

    if (!buffer_.extractFromLayer(layer, selPath)) {
        spdlog::warn("[MoveTool] Failed to extract selection pixels");
        return;
    }

    // Initialize transform state with buffer bounds
    transform_.setOriginalBounds(QRectF(buffer_.sourceRect()));

    spdlog::debug("[MoveTool] Extracted selection pixels, bounds: ({},{}) {}x{}",
                  buffer_.sourceRect().x(),
                  buffer_.sourceRect().y(),
                  buffer_.sourceRect().width(),
                  buffer_.sourceRect().height());

    // Determine effective copy mode: modifier override takes precedence over UI setting
    bool effectiveCopyMode = modifierOverride_ ? modifierCopyMode_ : (moveMode_ == MoveMode::Copy);

    // Only clear source if not in copy mode (Shift+Alt = copy, Ctrl+Alt = cut)
    if (!effectiveCopyMode) {
        buffer_.clearSourcePixels(layer);
    }
}

void MoveTool::continueStroke(const ToolInputEvent& event)
{
    proportionalScale_ = (event.modifiers & Qt::ShiftModifier) != 0;
    currentPos_ = event.canvasPos;

    // If scaling, update transform via handle drag (uses cached anchor)
    if (activeHandle_ != TransformHandle::None && !buffer_.isEmpty()) {
        transform_.updateFromHandleDrag(QPointF(currentPos_), proportionalScale_);
    } else if (isMovingSelection()) {
        // Free drag - update translation
        QPointF delta = QPointF(currentPos_ - startPos_);
        transform_.setTranslation(delta);
    }
}

void MoveTool::endStroke(const ToolInputEvent& event)
{
    currentPos_ = event.canvasPos;

    // Record delta - use transform translation if we have a buffer, else use raw positions
    if (!buffer_.isEmpty()) {
        lastDelta_ = transform_.translation().toPoint();
    } else {
        // No floating buffer = no selection or layer move, compute delta directly
        lastDelta_ = currentPos_ - startPos_;
    }

    // Keep floating buffer active after mouse release so user can:
    // - See and drag transform handles
    // - Continue adjusting position
    // - Press Enter to commit or Escape to cancel
    // This matches GIMP transform tool behavior.
    if (activeHandle_ != TransformHandle::None) {
        transform_.endHandleDrag();
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
            lastDelta_ = transform_.translation().toPoint();
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

void MoveTool::commitMove()
{
    if (!targetLayer_ || buffer_.isEmpty()) {
        clearFloatingState();
        modifierOverride_ = false;
        return;
    }

    // Determine effective copy mode: modifier override takes precedence over UI setting
    bool effectiveCopyMode = modifierOverride_ ? modifierCopyMode_ : (moveMode_ == MoveMode::Copy);

    QPoint offset = transform_.translation().toPoint();
    QSizeF scale = transform_.scale();
    bool hasScale = std::abs(scale.width() - 1.0) > 0.001 || std::abs(scale.height() - 1.0) > 0.001;

    spdlog::debug("[MoveTool] Committing move: offset=({},{}) scale=({:.2f},{:.2f}) copy={}",
                  offset.x(),
                  offset.y(),
                  scale.width(),
                  scale.height(),
                  effectiveCopyMode);

    // Calculate the bounding rect that covers both source and destination
    QRect srcRect = buffer_.sourceRect();
    QSize scaledSize = getScaledSize();
    QRect dstRect(srcRect.x() + offset.x(),
                  srcRect.y() + offset.y(),
                  scaledSize.width(),
                  scaledSize.height());
    QRect unionRect = srcRect.united(dstRect);

    spdlog::debug("[MoveTool] srcRect=({},{}) {}x{}, dstRect=({},{}) {}x{}, union=({},{}) {}x{}",
                  srcRect.x(),
                  srcRect.y(),
                  srcRect.width(),
                  srcRect.height(),
                  dstRect.x(),
                  dstRect.y(),
                  dstRect.width(),
                  dstRect.height(),
                  unionRect.x(),
                  unionRect.y(),
                  unionRect.width(),
                  unionRect.height());

    // Clip to layer bounds
    unionRect = unionRect.intersected(QRect(0, 0, targetLayer_->width(), targetLayer_->height()));

    // Check if destination is fully outside layer bounds
    QRect layerBounds(0, 0, targetLayer_->width(), targetLayer_->height());
    QRect dstClipped = dstRect.intersected(layerBounds);

    if (dstClipped.isEmpty()) {
        // Destination is completely off-canvas - warn user and keep buffer active
        spdlog::warn("[MoveTool] Cannot commit: selection is completely outside canvas. "
                     "Move it back on canvas before committing.");
        // Don't clear state - user can still move it back
        return;
    }

    // Create command and capture before state
    auto cmd = std::make_shared<MoveCommand>(targetLayer_, unionRect);

    // Get scaled buffer if needed
    std::vector<std::uint8_t> scaledBuf;
    if (hasScale) {
        scaledBuf = buffer_.getScaled(scale);
    }

    auto pasteBuffer = [&](QPoint pasteOffset, bool scaled) {
        auto& layerData = targetLayer_->data();
        int layerWidth = targetLayer_->width();
        int layerHeight = targetLayer_->height();
        constexpr int kPixelSize = 4;

        const std::vector<std::uint8_t>& srcBuf = scaled ? scaledBuf : buffer_.data();
        int srcW = scaled ? scaledSize.width() : buffer_.width();
        int srcH = scaled ? scaledSize.height() : buffer_.height();

        int dstX = buffer_.sourceRect().x() + pasteOffset.x();
        int dstY = buffer_.sourceRect().y() + pasteOffset.y();

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
        buffer_.pasteToLayer(targetLayer_, QPoint(0, 0));
        cmd->captureBeforeState();

        // Clear again and paste at new location
        buffer_.clearSourcePixels(targetLayer_);
        pasteBuffer(offset, hasScale);
        cmd->captureAfterState();
    }

    // Dispatch command
    if (commandBus_) {
        commandBus_->dispatch(cmd);
    }

    // Update selection outline to follow the moved/scaled pixels
    if (hasScale) {
        SelectionManager::instance().scaleSelection(scale, offset);
    } else {
        SelectionManager::instance().translateSelection(offset);
    }

    // Clip selection to document bounds to prevent color bleeding on next move
    // This is needed when selection was partially moved off-canvas
    if (dstClipped.width() < scaledSize.width() || dstClipped.height() < scaledSize.height()) {
        SelectionManager::instance().clipSelectionToDocument(
            targetLayer_->width(), targetLayer_->height());
    }

    clearFloatingState();
    modifierOverride_ = false;
}

void MoveTool::cancelMove()
{
    spdlog::debug("[MoveTool] Cancelling move, restoring original pixels");
    if (!targetLayer_ || buffer_.isEmpty()) {
        clearFloatingState();
        modifierOverride_ = false;
        return;
    }

    // Determine effective copy mode: modifier override takes precedence over UI setting
    bool effectiveCopyMode = modifierOverride_ ? modifierCopyMode_ : (moveMode_ == MoveMode::Copy);

    // Only restore if we were in cut mode (source was cleared)
    if (!effectiveCopyMode) {
        buffer_.pasteToLayer(targetLayer_, QPoint(0, 0));
    }

    clearFloatingState();
    modifierOverride_ = false;
}

void MoveTool::commitFloatingBuffer()
{
    if (!buffer_.isEmpty()) {
        commitMove();
    }
}

void MoveTool::cancelFloatingBuffer()
{
    if (!buffer_.isEmpty()) {
        cancelMove();
        state_ = ToolState::Idle;
    }
}

void MoveTool::clearFloatingState()
{
    buffer_.clear();
    transform_.reset();
    targetLayer_.reset();
    activeHandle_ = TransformHandle::None;
    proportionalScale_ = false;
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
    if (buffer_.isEmpty()) {
        return handles;
    }

    qreal handleSize = kHandleScreenSize / zoomLevel;
    auto floatHandles = transform_.getHandleRects(handleSize);

    for (const auto& fh : floatHandles) {
        handles.push_back(fh.toAlignedRect());
    }

    return handles;
}

TransformHandle MoveTool::hitTestHandle(const QPoint& pos, float zoomLevel) const
{
    if (buffer_.isEmpty()) {
        return TransformHandle::None;
    }
    qreal handleSize = kHandleScreenSize / zoomLevel;
    return transform_.hitTestHandle(QPointF(pos), handleSize);
}

QSize MoveTool::getScaledSize() const
{
    if (buffer_.isEmpty()) {
        return QSize();
    }
    return buffer_.getScaledSize(transform_.scale());
}

std::vector<std::uint8_t> MoveTool::getScaledBuffer() const
{
    if (buffer_.isEmpty()) {
        return {};
    }
    return buffer_.getScaled(transform_.scale());
}

}  // namespace gimp
