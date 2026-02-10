/**
 * @file rect_selection_tool.cpp
 * @brief Implementation of RectSelectTool.
 * @author Aless Tosi
 * @date 2026-02-09
 */

#include "core/tools/rect_selection_tool.h"

#include "core/commands/selection_command.h"
#include "core/document.h"
#include "core/selection_manager.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace gimp {

namespace {

// Handle size in screen pixels (constant regardless of zoom)
constexpr float kHandleScreenSize = 8.0F;

QRectF clampRectToDocument(const QRectF& rect, const Document* document)
{
    if (!document) {
        return rect;
    }
    const QRectF bounds(0.0, 0.0, document->width(), document->height());
    return rect.intersected(bounds);
}

}  // namespace

QPainterPath RectSelectTool::buildRectPath(const QRectF& rect) const
{
    QRectF clampedRect = clampRectToDocument(rect, document().get());
    QPainterPath path;
    if (clampedRect.isValid() && clampedRect.width() > 0.0 && clampedRect.height() > 0.0) {
        path.addRect(clampedRect);
    }
    return path;
}

std::vector<QRectF> RectSelectTool::getHandleRects(float zoomLevel) const
{
    std::vector<QRectF> handles;
    if (currentBounds_.isEmpty()) {
        return handles;
    }

    // Handle size in canvas coordinates (screen-space constant)
    float handleSize = kHandleScreenSize / zoomLevel;
    float halfSize = handleSize / 2.0F;

    double left = currentBounds_.left();
    double top = currentBounds_.top();
    double right = currentBounds_.right();
    double bottom = currentBounds_.bottom();
    double midX = (left + right) / 2.0;
    double midY = (top + bottom) / 2.0;

    // Order: TopLeft, Top, TopRight, Right, BottomRight, Bottom, BottomLeft, Left
    handles.emplace_back(left - halfSize, top - halfSize, handleSize, handleSize);
    handles.emplace_back(midX - halfSize, top - halfSize, handleSize, handleSize);
    handles.emplace_back(right - halfSize, top - halfSize, handleSize, handleSize);
    handles.emplace_back(right - halfSize, midY - halfSize, handleSize, handleSize);
    handles.emplace_back(right - halfSize, bottom - halfSize, handleSize, handleSize);
    handles.emplace_back(midX - halfSize, bottom - halfSize, handleSize, handleSize);
    handles.emplace_back(left - halfSize, bottom - halfSize, handleSize, handleSize);
    handles.emplace_back(left - halfSize, midY - halfSize, handleSize, handleSize);

    return handles;
}

SelectionHandle RectSelectTool::hitTestHandle(const QPoint& pos, float zoomLevel) const
{
    auto handles = getHandleRects(zoomLevel);
    if (handles.empty()) {
        return SelectionHandle::None;
    }

    static constexpr std::array<SelectionHandle, 8> kHandleTypes = {SelectionHandle::TopLeft,
                                                                    SelectionHandle::Top,
                                                                    SelectionHandle::TopRight,
                                                                    SelectionHandle::Right,
                                                                    SelectionHandle::BottomRight,
                                                                    SelectionHandle::Bottom,
                                                                    SelectionHandle::BottomLeft,
                                                                    SelectionHandle::Left};

    QPointF posF(pos);
    for (size_t i = 0; i < handles.size(); ++i) {
        if (handles[i].contains(posF)) {
            return kHandleTypes[i];
        }
    }

    return SelectionHandle::None;
}

QPointF RectSelectTool::getAnchorForHandle(SelectionHandle handle) const
{
    switch (handle) {
        case SelectionHandle::TopLeft:
            return currentBounds_.bottomRight();
        case SelectionHandle::Top:
            return {currentBounds_.center().x(), currentBounds_.bottom()};
        case SelectionHandle::TopRight:
            return currentBounds_.bottomLeft();
        case SelectionHandle::Right:
            return {currentBounds_.left(), currentBounds_.center().y()};
        case SelectionHandle::BottomRight:
            return currentBounds_.topLeft();
        case SelectionHandle::Bottom:
            return {currentBounds_.center().x(), currentBounds_.top()};
        case SelectionHandle::BottomLeft:
            return currentBounds_.topRight();
        case SelectionHandle::Left:
            return {currentBounds_.right(), currentBounds_.center().y()};
        default:
            return currentBounds_.center();
    }
}

void RectSelectTool::finalizeSelection()
{
    commitSelectionCommand();
    phase_ = SelectionPhase::Idle;
    activeHandle_ = SelectionHandle::None;
    // Selection is already applied, just clear adjusting state
}

void RectSelectTool::beginStroke(const ToolInputEvent& event)
{
    // Use zoom from event, fallback to 1.0
    float zoomLevel = event.zoomLevel > 0.0F ? event.zoomLevel : 1.0F;

    // If we have an active selection in Adjusting phase, check for handle hit
    if (phase_ == SelectionPhase::Adjusting) {
        // Sync bounds from SelectionManager in case selection was transformed externally
        const auto& selPath = SelectionManager::instance().selectionPath();
        if (!selPath.isEmpty()) {
            currentBounds_ = selPath.boundingRect();
        }

        SelectionHandle handle = hitTestHandle(event.canvasPos, zoomLevel);
        if (handle != SelectionHandle::None) {
            // Start resizing via handle - begin command for resize operation
            beginSelectionCommand("Resize Selection");
            activeHandle_ = handle;
            startPos_ = event.canvasPos;
            scaleAnchor_ = getAnchorForHandle(handle);
            return;
        }

        // Click was not on a handle - check if inside selection
        if (currentBounds_.contains(QPointF(event.canvasPos))) {
            // Could implement move here, but for now just finalize
            finalizeSelection();
            return;
        }

        // Click outside selection with no handle - start new selection
        commitSelectionCommand();
        finalizeSelection();
    }

    // Start creating a new selection
    beginSelectionCommand("Rectangle Select");
    phase_ = SelectionPhase::Creating;
    startPos_ = event.canvasPos;
    currentMode_ = resolveSelectionMode(event.modifiers);
    currentBounds_ = QRectF(event.canvasPos, QSizeF(0, 0));

    auto previewPath = buildRectPath(currentBounds_);
    SelectionManager::instance().setPreview(previewPath, currentMode_);
}

void RectSelectTool::continueStroke(const ToolInputEvent& event)
{
    if (phase_ == SelectionPhase::Creating) {
        // Building new selection with modifiers
        const bool centerOut = (event.modifiers & Qt::AltModifier) != 0;
        const bool constrainSquare = (event.modifiers & Qt::ShiftModifier) != 0;

        QPointF start(startPos_);
        QPointF current(event.canvasPos);

        if (centerOut) {
            double dx = current.x() - start.x();
            double dy = current.y() - start.y();
            double rx = std::abs(dx);
            double ry = std::abs(dy);
            if (constrainSquare) {
                double r = std::max(rx, ry);
                rx = r;
                ry = r;
            }
            currentBounds_ = QRectF(start.x() - rx, start.y() - ry, rx * 2.0, ry * 2.0);
        } else {
            double dx = current.x() - start.x();
            double dy = current.y() - start.y();
            if (constrainSquare) {
                double size = std::max(std::abs(dx), std::abs(dy));
                dx = (dx < 0.0) ? -size : size;
                dy = (dy < 0.0) ? -size : size;
            }
            currentBounds_ = QRectF(start, QSizeF(dx, dy)).normalized();
        }

        auto previewPath = buildRectPath(currentBounds_);
        SelectionManager::instance().setPreview(previewPath, currentMode_);
    } else if (phase_ == SelectionPhase::Adjusting && activeHandle_ != SelectionHandle::None) {
        // Resizing selection via handle
        QPointF mousePos(event.canvasPos);

        // Calculate new bounds based on handle being dragged
        double newLeft = currentBounds_.left();
        double newTop = currentBounds_.top();
        double newRight = currentBounds_.right();
        double newBottom = currentBounds_.bottom();

        switch (activeHandle_) {
            case SelectionHandle::TopLeft:
                newLeft = mousePos.x();
                newTop = mousePos.y();
                break;
            case SelectionHandle::Top:
                newTop = mousePos.y();
                break;
            case SelectionHandle::TopRight:
                newRight = mousePos.x();
                newTop = mousePos.y();
                break;
            case SelectionHandle::Right:
                newRight = mousePos.x();
                break;
            case SelectionHandle::BottomRight:
                newRight = mousePos.x();
                newBottom = mousePos.y();
                break;
            case SelectionHandle::Bottom:
                newBottom = mousePos.y();
                break;
            case SelectionHandle::BottomLeft:
                newLeft = mousePos.x();
                newBottom = mousePos.y();
                break;
            case SelectionHandle::Left:
                newLeft = mousePos.x();
                break;
            case SelectionHandle::None:
                break;
        }

        // Normalize to handle inverted dragging
        currentBounds_ =
            QRectF(QPointF(newLeft, newTop), QPointF(newRight, newBottom)).normalized();

        // Update selection
        auto path = buildRectPath(currentBounds_);
        SelectionManager::instance().applySelection(path, currentMode_, SelectionType::Rectangle);
    }
}

void RectSelectTool::endStroke(const ToolInputEvent& event)
{
    (void)event;

    if (phase_ == SelectionPhase::Creating) {
        // Finish creating, enter Adjusting phase
        auto path = buildRectPath(currentBounds_);
        SelectionManager::instance().applySelection(path, currentMode_, SelectionType::Rectangle);
        SelectionManager::instance().clearPreview();
        commitSelectionCommand();

        if (!currentBounds_.isEmpty()) {
            phase_ = SelectionPhase::Adjusting;
        } else {
            phase_ = SelectionPhase::Idle;
        }
    } else if (phase_ == SelectionPhase::Adjusting) {
        // Finish handle drag, commit and stay in Adjusting
        if (activeHandle_ != SelectionHandle::None) {
            commitSelectionCommand();
        }
        activeHandle_ = SelectionHandle::None;
    }
}

void RectSelectTool::cancelStroke()
{
    resetToIdle();
}

void RectSelectTool::resetToIdle()
{
    cancelSelectionOperation();
    phase_ = SelectionPhase::Idle;
    currentBounds_ = QRectF();
    activeHandle_ = SelectionHandle::None;
}

void RectSelectTool::onDeactivate()
{
    resetToIdle();
    Tool::onDeactivate();
}

bool RectSelectTool::onKeyPress(Qt::Key key, Qt::KeyboardModifiers /*modifiers*/)
{
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        if (phase_ == SelectionPhase::Adjusting) {
            finalizeSelection();
            return true;
        }
    } else if (key == Qt::Key_Escape) {
        if (phase_ == SelectionPhase::Adjusting) {
            // Cancel selection entirely - create command for the clear
            beginSelectionCommand("Deselect");
            SelectionManager::instance().clear();
            commitSelectionCommand();
            phase_ = SelectionPhase::Idle;
            currentBounds_ = QRectF();
            return true;
        }
    }
    return false;
}

}  // namespace gimp
