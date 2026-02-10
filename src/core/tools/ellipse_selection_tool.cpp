/**
 * @file ellipse_selection_tool.cpp
 * @brief Implementation of EllipseSelectTool with handle-based resize.
 * @author Aless Tosi
 * @date 2026-02-03
 */

#include "core/tools/ellipse_selection_tool.h"

#include "core/commands/selection_command.h"
#include "core/document.h"
#include "core/selection_manager.h"

#include <QRectF>

#include <algorithm>
#include <cmath>

namespace gimp {

namespace {

QRectF clampRectToDocument(const QRectF& rect, const Document* document)
{
    if (!document) {
        return rect;
    }

    const QRectF bounds(0.0, 0.0, document->width(), document->height());
    return rect.intersected(bounds);
}

}  // namespace

QPainterPath EllipseSelectTool::buildEllipsePath(const QPoint& start,
                                                 const QPoint& current,
                                                 Qt::KeyboardModifiers modifiers) const
{
    const bool centerOut = (modifiers & Qt::AltModifier) != 0;
    const bool constrainCircle = (modifiers & Qt::ShiftModifier) != 0;

    QRectF rect;

    if (centerOut) {
        const float dx = static_cast<float>(current.x() - start.x());
        const float dy = static_cast<float>(current.y() - start.y());
        float rx = std::abs(dx);
        float ry = std::abs(dy);
        if (constrainCircle) {
            const float r = std::max(rx, ry);
            rx = r;
            ry = r;
        }
        const float startX = static_cast<float>(start.x());
        const float startY = static_cast<float>(start.y());
        rect = QRectF(startX - rx, startY - ry, rx * 2.0F, ry * 2.0F);
    } else {
        float dx = static_cast<float>(current.x() - start.x());
        float dy = static_cast<float>(current.y() - start.y());
        if (constrainCircle) {
            const float size = std::max(std::abs(dx), std::abs(dy));
            dx = (dx < 0.0F) ? -size : size;
            dy = (dy < 0.0F) ? -size : size;
        }
        rect = QRectF(start.x(), start.y(), dx, dy).normalized();
    }

    rect = clampRectToDocument(rect, document().get());

    QPainterPath path;
    if (rect.isValid() && rect.width() > 0.0F && rect.height() > 0.0F) {
        path.addEllipse(rect);
    }
    return path;
}

std::array<QRectF, 8> EllipseSelectTool::getHandleRects(float zoomLevel) const
{
    std::array<QRectF, 8> handles{};
    if (phase_ != EllipseSelectionPhase::Adjusting || currentBounds_.isEmpty()) {
        return handles;
    }

    // Handle size in canvas coordinates (constant screen size)
    const float handleSize = kEllipseHandleScreenSize / zoomLevel;
    const float half = handleSize / 2.0F;

    const qreal left = currentBounds_.left();
    const qreal right = currentBounds_.right();
    const qreal top = currentBounds_.top();
    const qreal bottom = currentBounds_.bottom();
    const qreal cx = currentBounds_.center().x();
    const qreal cy = currentBounds_.center().y();

    // Order: TopLeft, Top, TopRight, Right, BottomRight, Bottom, BottomLeft, Left
    handles[0] = QRectF(left - half, top - half, handleSize, handleSize);
    handles[1] = QRectF(cx - half, top - half, handleSize, handleSize);
    handles[2] = QRectF(right - half, top - half, handleSize, handleSize);
    handles[3] = QRectF(right - half, cy - half, handleSize, handleSize);
    handles[4] = QRectF(right - half, bottom - half, handleSize, handleSize);
    handles[5] = QRectF(cx - half, bottom - half, handleSize, handleSize);
    handles[6] = QRectF(left - half, bottom - half, handleSize, handleSize);
    handles[7] = QRectF(left - half, cy - half, handleSize, handleSize);

    return handles;
}

EllipseSelectionHandle EllipseSelectTool::hitTestHandle(const QPointF& pos, float zoomLevel) const
{
    auto handles = getHandleRects(zoomLevel);

    // Check in reverse order (corners first for priority)
    if (handles[4].contains(pos)) {
        return EllipseSelectionHandle::BottomRight;
    }
    if (handles[0].contains(pos)) {
        return EllipseSelectionHandle::TopLeft;
    }
    if (handles[2].contains(pos)) {
        return EllipseSelectionHandle::TopRight;
    }
    if (handles[6].contains(pos)) {
        return EllipseSelectionHandle::BottomLeft;
    }
    if (handles[1].contains(pos)) {
        return EllipseSelectionHandle::Top;
    }
    if (handles[5].contains(pos)) {
        return EllipseSelectionHandle::Bottom;
    }
    if (handles[3].contains(pos)) {
        return EllipseSelectionHandle::Right;
    }
    if (handles[7].contains(pos)) {
        return EllipseSelectionHandle::Left;
    }

    return EllipseSelectionHandle::None;
}

QPointF EllipseSelectTool::getAnchorForHandle(EllipseSelectionHandle handle) const
{
    switch (handle) {
        case EllipseSelectionHandle::TopLeft:
            return currentBounds_.bottomRight();
        case EllipseSelectionHandle::Top:
            return {currentBounds_.center().x(), currentBounds_.bottom()};
        case EllipseSelectionHandle::TopRight:
            return currentBounds_.bottomLeft();
        case EllipseSelectionHandle::Right:
            return {currentBounds_.left(), currentBounds_.center().y()};
        case EllipseSelectionHandle::BottomRight:
            return currentBounds_.topLeft();
        case EllipseSelectionHandle::Bottom:
            return {currentBounds_.center().x(), currentBounds_.top()};
        case EllipseSelectionHandle::BottomLeft:
            return currentBounds_.topRight();
        case EllipseSelectionHandle::Left:
            return {currentBounds_.right(), currentBounds_.center().y()};
        default:
            return currentBounds_.center();
    }
}

bool EllipseSelectTool::onKeyPress(Qt::Key key, Qt::KeyboardModifiers /*modifiers*/)
{
    if (phase_ != EllipseSelectionPhase::Adjusting) {
        return false;
    }

    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        // Finalize: apply current bounds as selection (already applied, just commit)
        commitSelectionCommand();
        QPainterPath path;
        if (currentBounds_.isValid() && currentBounds_.width() > 0 && currentBounds_.height() > 0) {
            path.addEllipse(currentBounds_);
        }
        SelectionManager::instance().applySelection(
            path, SelectionMode::Replace, SelectionType::Ellipse);
        SelectionManager::instance().clearPreview();
        phase_ = EllipseSelectionPhase::Idle;
        currentBounds_ = QRectF();
        return true;
    }

    if (key == Qt::Key_Escape) {
        // Cancel: clear selection and return to idle - create command for clear
        beginSelectionCommand("Deselect");
        SelectionManager::instance().clear();
        commitSelectionCommand();
        SelectionManager::instance().clearPreview();
        phase_ = EllipseSelectionPhase::Idle;
        currentBounds_ = QRectF();
        return true;
    }

    return false;
}

void EllipseSelectTool::beginStroke(const ToolInputEvent& event)
{
    const float zoomLevel = event.zoomLevel > 0.0F ? event.zoomLevel : 1.0F;
    const QPointF canvasPos(event.canvasPos);

    // In Adjusting phase, check for handle hits first
    if (phase_ == EllipseSelectionPhase::Adjusting) {
        // Sync bounds from SelectionManager in case selection was transformed externally
        const auto& selPath = SelectionManager::instance().selectionPath();
        if (!selPath.isEmpty()) {
            currentBounds_ = selPath.boundingRect();
        }

        activeHandle_ = hitTestHandle(canvasPos, zoomLevel);
        if (activeHandle_ != EllipseSelectionHandle::None) {
            // Start resize from handle - begin command for resize operation
            beginSelectionCommand("Resize Selection");
            scaleAnchor_ = getAnchorForHandle(activeHandle_);
            originalBounds_ = currentBounds_;
            return;
        }

        // Click outside handles - finalize current selection and start new
        commitSelectionCommand();
        QPainterPath path;
        if (currentBounds_.isValid() && currentBounds_.width() > 0 && currentBounds_.height() > 0) {
            path.addEllipse(currentBounds_);
        }
        SelectionManager::instance().applySelection(
            path, SelectionMode::Replace, SelectionType::Ellipse);
        SelectionManager::instance().clearPreview();
        currentBounds_ = QRectF();
    }

    // Start new selection (Creating phase)
    beginSelectionCommand("Ellipse Select");
    phase_ = EllipseSelectionPhase::Creating;
    startPos_ = event.canvasPos;
    currentPos_ = event.canvasPos;
    currentMode_ = resolveSelectionMode(event.modifiers);
    activeHandle_ = EllipseSelectionHandle::None;

    auto previewPath = buildEllipsePath(startPos_, currentPos_, event.modifiers);
    SelectionManager::instance().setPreview(previewPath, currentMode_);
}

void EllipseSelectTool::continueStroke(const ToolInputEvent& event)
{
    const QPointF canvasPos(event.canvasPos);

    // Handle resize during Adjusting phase
    if (phase_ == EllipseSelectionPhase::Adjusting &&
        activeHandle_ != EllipseSelectionHandle::None) {
        // Compute new bounds from anchor + current mouse
        qreal newLeft = 0;
        qreal newTop = 0;
        qreal newRight = 0;
        qreal newBottom = 0;

        switch (activeHandle_) {
            case EllipseSelectionHandle::TopLeft:
                newLeft = canvasPos.x();
                newTop = canvasPos.y();
                newRight = scaleAnchor_.x();
                newBottom = scaleAnchor_.y();
                break;
            case EllipseSelectionHandle::Top:
                newLeft = currentBounds_.left();
                newTop = canvasPos.y();
                newRight = currentBounds_.right();
                newBottom = scaleAnchor_.y();
                break;
            case EllipseSelectionHandle::TopRight:
                newLeft = scaleAnchor_.x();
                newTop = canvasPos.y();
                newRight = canvasPos.x();
                newBottom = scaleAnchor_.y();
                break;
            case EllipseSelectionHandle::Right:
                newLeft = scaleAnchor_.x();
                newTop = currentBounds_.top();
                newRight = canvasPos.x();
                newBottom = currentBounds_.bottom();
                break;
            case EllipseSelectionHandle::BottomRight:
                newLeft = scaleAnchor_.x();
                newTop = scaleAnchor_.y();
                newRight = canvasPos.x();
                newBottom = canvasPos.y();
                break;
            case EllipseSelectionHandle::Bottom:
                newLeft = currentBounds_.left();
                newTop = scaleAnchor_.y();
                newRight = currentBounds_.right();
                newBottom = canvasPos.y();
                break;
            case EllipseSelectionHandle::BottomLeft:
                newLeft = canvasPos.x();
                newTop = scaleAnchor_.y();
                newRight = scaleAnchor_.x();
                newBottom = canvasPos.y();
                break;
            case EllipseSelectionHandle::Left:
                newLeft = canvasPos.x();
                newTop = currentBounds_.top();
                newRight = scaleAnchor_.x();
                newBottom = currentBounds_.bottom();
                break;
            default:
                return;
        }

        currentBounds_ =
            QRectF(QPointF(newLeft, newTop), QPointF(newRight, newBottom)).normalized();

        // Update preview
        QPainterPath previewPath;
        if (currentBounds_.isValid() && currentBounds_.width() > 0 && currentBounds_.height() > 0) {
            previewPath.addEllipse(currentBounds_);
        }
        SelectionManager::instance().setPreview(previewPath, SelectionMode::Replace);
        return;
    }

    // Creating phase - update ellipse preview
    if (phase_ == EllipseSelectionPhase::Creating) {
        currentPos_ = event.canvasPos;
        auto previewPath = buildEllipsePath(startPos_, currentPos_, event.modifiers);
        SelectionManager::instance().setPreview(previewPath, currentMode_);
    }
}

void EllipseSelectTool::endStroke(const ToolInputEvent& event)
{
    // End handle resize
    if (phase_ == EllipseSelectionPhase::Adjusting &&
        activeHandle_ != EllipseSelectionHandle::None) {
        // Apply the final selection state from the resize
        QPainterPath path;
        if (currentBounds_.isValid() && currentBounds_.width() > 0 && currentBounds_.height() > 0) {
            path.addEllipse(currentBounds_);
        }
        SelectionManager::instance().applySelection(
            path, SelectionMode::Replace, SelectionType::Ellipse);
        SelectionManager::instance().clearPreview();
        commitSelectionCommand();
        activeHandle_ = EllipseSelectionHandle::None;
        // Stay in Adjusting phase - handles remain visible
        return;
    }

    // End Creating phase - transition to Adjusting
    if (phase_ == EllipseSelectionPhase::Creating) {
        currentPos_ = event.canvasPos;
        auto path = buildEllipsePath(startPos_, currentPos_, event.modifiers);

        // Store current bounds for handle display
        currentBounds_ = path.boundingRect();

        if (currentBounds_.isValid() && currentBounds_.width() > 0 && currentBounds_.height() > 0) {
            // Apply the selection immediately but keep handles visible
            SelectionManager::instance().applySelection(path, currentMode_, SelectionType::Ellipse);
            commitSelectionCommand();
            phase_ = EllipseSelectionPhase::Adjusting;
        } else {
            // Invalid selection - back to idle
            pendingCommand_.reset();
            phase_ = EllipseSelectionPhase::Idle;
        }

        SelectionManager::instance().clearPreview();
    }
}

void EllipseSelectTool::cancelStroke()
{
    resetToIdle();
}

void EllipseSelectTool::resetToIdle()
{
    cancelSelectionOperation();
    phase_ = EllipseSelectionPhase::Idle;
    currentBounds_ = QRectF();
    activeHandle_ = EllipseSelectionHandle::None;
    originalBounds_ = QRectF();
}

void EllipseSelectTool::onDeactivate()
{
    resetToIdle();
    Tool::onDeactivate();
}

}  // namespace gimp
