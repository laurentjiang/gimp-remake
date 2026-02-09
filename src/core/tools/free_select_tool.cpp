/**
 * @file free_select_tool.cpp
 * @brief Implementation of FreeSelectTool.
 * @author Laurent Jiang
 * @date 2026-02-09
 */

#include "core/tools/free_select_tool.h"

#include "core/document.h"
#include "core/selection_manager.h"

#include <cmath>

namespace gimp {

SelectionMode FreeSelectTool::resolveSelectionMode(Qt::KeyboardModifiers modifiers)
{
    if ((modifiers & Qt::ControlModifier) != 0) {
        if ((modifiers & Qt::AltModifier) != 0) {
            return SelectionMode::Subtract;
        }
        return SelectionMode::Add;
    }
    return SelectionMode::Replace;
}

QPainterPath FreeSelectTool::buildPath(bool close) const
{
    QPainterPath path;

    if (points_.empty()) {
        return path;
    }

    path.moveTo(points_.front());
    for (std::size_t i = 1; i < points_.size(); ++i) {
        path.lineTo(points_[i]);
    }

    if (close && points_.size() >= 3) {
        path.closeSubpath();
    }

    return path;
}

void FreeSelectTool::beginStroke(const ToolInputEvent& event)
{
    points_.clear();
    points_.emplace_back(event.canvasPos);
    currentMode_ = resolveSelectionMode(event.modifiers);

    auto previewPath = buildPath(false);
    SelectionManager::instance().setPreview(previewPath, currentMode_);
}

void FreeSelectTool::continueStroke(const ToolInputEvent& event)
{
    // Only add point if it's far enough from the last point
    // to avoid excessive point density during fast strokes
    if (!points_.empty()) {
        const QPointF& lastPoint = points_.back();
        const float dx =
            static_cast<float>(event.canvasPos.x()) - static_cast<float>(lastPoint.x());
        const float dy =
            static_cast<float>(event.canvasPos.y()) - static_cast<float>(lastPoint.y());
        const float distance = std::sqrt(dx * dx + dy * dy);

        if (distance < kMinPointDistance) {
            return;
        }
    }

    points_.emplace_back(event.canvasPos);

    auto previewPath = buildPath(false);
    SelectionManager::instance().setPreview(previewPath, currentMode_);
}

void FreeSelectTool::endStroke(const ToolInputEvent& event)
{
    // Add final point if different from last
    if (!points_.empty()) {
        const QPointF& lastPoint = points_.back();
        if (lastPoint != QPointF(event.canvasPos)) {
            points_.emplace_back(event.canvasPos);
        }
    }

    // Need at least 3 points to form a valid selection polygon
    if (points_.size() >= 3) {
        auto path = buildPath(true);
        SelectionManager::instance().applySelection(path, currentMode_);
    }

    SelectionManager::instance().clearPreview();
    points_.clear();
}

void FreeSelectTool::cancelStroke()
{
    SelectionManager::instance().clearPreview();
    points_.clear();
}

}  // namespace gimp
