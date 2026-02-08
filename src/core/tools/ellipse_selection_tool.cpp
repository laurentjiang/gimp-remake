/**
 * @file ellipse_selection_tool.cpp
 * @brief Implementation of EllipseSelectTool.
 * @author Aless Tosi
 * @date 2026-02-03
 */

#include "core/tools/ellipse_selection_tool.h"

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

SelectionMode EllipseSelectTool::resolveSelectionMode(Qt::KeyboardModifiers modifiers)
{
    if ((modifiers & Qt::ControlModifier) != 0) {
        if ((modifiers & Qt::AltModifier) != 0) {
            return SelectionMode::Subtract;
        }
        return SelectionMode::Add;
    }
    return SelectionMode::Replace;
}

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

void EllipseSelectTool::beginStroke(const ToolInputEvent& event)
{
    startPos_ = event.canvasPos;
    currentPos_ = event.canvasPos;
    currentMode_ = resolveSelectionMode(event.modifiers);

    auto previewPath = buildEllipsePath(startPos_, currentPos_, event.modifiers);
    SelectionManager::instance().setPreview(previewPath, currentMode_);
}

void EllipseSelectTool::continueStroke(const ToolInputEvent& event)
{
    currentPos_ = event.canvasPos;
    auto previewPath = buildEllipsePath(startPos_, currentPos_, event.modifiers);
    SelectionManager::instance().setPreview(previewPath, currentMode_);
}

void EllipseSelectTool::endStroke(const ToolInputEvent& event)
{
    currentPos_ = event.canvasPos;
    auto path = buildEllipsePath(startPos_, currentPos_, event.modifiers);

    SelectionManager::instance().applySelection(path, currentMode_);
    SelectionManager::instance().clearPreview();
}

void EllipseSelectTool::cancelStroke()
{
    SelectionManager::instance().clearPreview();
}

}  // namespace gimp
