/**
 * @file transform_state.cpp
 * @brief Implementation of TransformState.
 * @author Laurent Jiang
 * @date 2026-02-10
 */

#include "core/transform_state.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <array>
#include <cmath>

namespace gimp {

TransformState::TransformState(const QRectF& bounds) : originalBounds_(bounds) {}

bool TransformState::isIdentity() const
{
    constexpr qreal kEpsilon = 0.0001;
    return std::abs(translation_.x()) < kEpsilon && std::abs(translation_.y()) < kEpsilon &&
           std::abs(scale_.width() - 1.0) < kEpsilon &&
           std::abs(scale_.height() - 1.0) < kEpsilon && std::abs(rotation_) < kEpsilon;
}

void TransformState::reset()
{
    translation_ = QPointF(0.0, 0.0);
    scale_ = QSizeF(1.0, 1.0);
    rotation_ = 0.0;
    activeHandle_ = TransformHandle::None;
    scaleAnchor_ = QPointF();
}

void TransformState::setOriginalBounds(const QRectF& bounds)
{
    originalBounds_ = bounds;
    reset();
}

QTransform TransformState::matrix() const
{
    return buildMatrix();
}

QTransform TransformState::buildMatrix() const
{
    // Build transform: translate to origin, scale, rotate, translate back + offset
    QPointF center = originalBounds_.center();

    QTransform transform;
    // Apply translation
    transform.translate(translation_.x(), translation_.y());
    // Transform around center for scale/rotation
    transform.translate(center.x(), center.y());
    transform.rotate(rotation_);
    transform.scale(scale_.width(), scale_.height());
    transform.translate(-center.x(), -center.y());

    return transform;
}

QRectF TransformState::transformedBounds() const
{
    // Compute directly to match updateFromHandleDrag() math
    // The matrix uses center-based scaling, but handle drag uses top-left based translation
    qreal scaledW = originalBounds_.width() * scale_.width();
    qreal scaledH = originalBounds_.height() * scale_.height();
    return {originalBounds_.left() + translation_.x(),
            originalBounds_.top() + translation_.y(),
            scaledW,
            scaledH};
}

void TransformState::translate(const QPointF& offset)
{
    translation_ += offset;
}

void TransformState::setTranslation(const QPointF& offset)
{
    translation_ = offset;
}

void TransformState::scaleUniform(qreal factor, const QPointF& anchor)
{
    scaleNonUniform(factor, factor, anchor);
}

void TransformState::scaleNonUniform(qreal sx, qreal sy, const QPointF& anchor)
{
    // Scale relative to anchor point
    // New position = anchor + (old_position - anchor) * scale
    // This means translation needs to change to keep anchor fixed

    QPointF center = originalBounds_.center();
    QPointF currentCenter = center + translation_;

    // Apply scale to current offset from anchor
    QPointF offset = currentCenter - anchor;
    QPointF newOffset =
        QPointF(offset.x() * sx / scale_.width(), offset.y() * sy / scale_.height());
    QPointF newCenter = anchor + newOffset;

    translation_ = newCenter - center;
    scale_ = QSizeF(scale_.width() * sx / scale_.width(), scale_.height() * sy / scale_.height());
}

void TransformState::setScale(const QSizeF& scale)
{
    scale_ = scale;
}

void TransformState::rotate(qreal degrees, const QPointF& anchor)
{
    (void)anchor;  // Rotation implementation for future use
    rotation_ += degrees;
}

void TransformState::setRotation(qreal degrees)
{
    rotation_ = degrees;
}

std::vector<QPointF> TransformState::getHandlePositions() const
{
    QRectF bounds = transformedBounds();

    std::vector<QPointF> positions;
    positions.reserve(8);

    qreal left = bounds.left();
    qreal right = bounds.right();
    qreal top = bounds.top();
    qreal bottom = bounds.bottom();
    qreal midX = bounds.center().x();
    qreal midY = bounds.center().y();

    // Order: TopLeft, Top, TopRight, Right, BottomRight, Bottom, BottomLeft, Left
    positions.emplace_back(left, top);
    positions.emplace_back(midX, top);
    positions.emplace_back(right, top);
    positions.emplace_back(right, midY);
    positions.emplace_back(right, bottom);
    positions.emplace_back(midX, bottom);
    positions.emplace_back(left, bottom);
    positions.emplace_back(left, midY);

    return positions;
}

std::vector<QRectF> TransformState::getHandleRects(qreal handleSize) const
{
    std::vector<QRectF> handles;
    handles.reserve(8);

    if (originalBounds_.isEmpty()) {
        return handles;
    }

    qreal halfSize = handleSize / 2.0;
    auto positions = getHandlePositions();

    for (const auto& pos : positions) {
        handles.emplace_back(pos.x() - halfSize, pos.y() - halfSize, handleSize, handleSize);
    }

    return handles;
}

TransformHandle TransformState::hitTestHandle(const QPointF& pos, qreal handleSize) const
{
    auto handles = getHandleRects(handleSize);
    if (handles.empty()) {
        return TransformHandle::None;
    }

    static constexpr std::array<TransformHandle, 8> kHandleTypes = {TransformHandle::TopLeft,
                                                                     TransformHandle::Top,
                                                                     TransformHandle::TopRight,
                                                                     TransformHandle::Right,
                                                                     TransformHandle::BottomRight,
                                                                     TransformHandle::Bottom,
                                                                     TransformHandle::BottomLeft,
                                                                     TransformHandle::Left};

    for (std::size_t i = 0; i < handles.size(); ++i) {
        if (handles[i].contains(pos)) {
            return kHandleTypes[i];
        }
    }

    return TransformHandle::None;
}

QPointF TransformState::getAnchorForHandle(TransformHandle handle) const
{
    auto positions = getHandlePositions();
    if (positions.size() < 8) {
        return transformedBounds().center();
    }

    // Return the opposite handle position as anchor
    switch (handle) {
        case TransformHandle::TopLeft:
            return positions[4];  // BottomRight
        case TransformHandle::Top:
            return positions[5];  // Bottom
        case TransformHandle::TopRight:
            return positions[6];  // BottomLeft
        case TransformHandle::Right:
            return positions[7];  // Left
        case TransformHandle::BottomRight:
            return positions[0];  // TopLeft
        case TransformHandle::Bottom:
            return positions[1];  // Top
        case TransformHandle::BottomLeft:
            return positions[2];  // TopRight
        case TransformHandle::Left:
            return positions[3];  // Right
        default:
            return transformedBounds().center();
    }
}

void TransformState::beginHandleDrag(TransformHandle handle)
{
    activeHandle_ = handle;
    // Capture anchor position at drag start - this stays fixed during the entire drag
    scaleAnchor_ = getAnchorForHandle(handle);
    spdlog::debug("[TransformState] Begin handle drag: handle={} anchor=({:.1f},{:.1f})",
                  static_cast<int>(handle),
                  scaleAnchor_.x(),
                  scaleAnchor_.y());
}

void TransformState::endHandleDrag()
{
    activeHandle_ = TransformHandle::None;
    scaleAnchor_ = QPointF();
}

void TransformState::updateFromHandleDrag(const QPointF& newPos, bool proportional)
{
    if (originalBounds_.isEmpty() || activeHandle_ == TransformHandle::None) {
        return;
    }

    // Use the anchor captured at drag start (scaleAnchor_), not recalculated each frame
    // This ensures the anchor stays fixed throughout the drag operation

    // Calculate new scale based on mouse distance from anchor vs original distance
    qreal origW = originalBounds_.width();
    qreal origH = originalBounds_.height();

    // Compute scale factors based on which handle is being dragged
    qreal sx = scale_.width();
    qreal sy = scale_.height();

    switch (activeHandle_) {
        case TransformHandle::TopLeft:
            // Mouse is at top-left, anchor is bottom-right
            sx = (scaleAnchor_.x() - newPos.x()) / origW;
            sy = (scaleAnchor_.y() - newPos.y()) / origH;
            break;
        case TransformHandle::Top:
            // Only Y changes
            sy = (scaleAnchor_.y() - newPos.y()) / origH;
            break;
        case TransformHandle::TopRight:
            sx = (newPos.x() - scaleAnchor_.x()) / origW;
            sy = (scaleAnchor_.y() - newPos.y()) / origH;
            break;
        case TransformHandle::Right:
            sx = (newPos.x() - scaleAnchor_.x()) / origW;
            break;
        case TransformHandle::BottomRight:
            sx = (newPos.x() - scaleAnchor_.x()) / origW;
            sy = (newPos.y() - scaleAnchor_.y()) / origH;
            break;
        case TransformHandle::Bottom:
            sy = (newPos.y() - scaleAnchor_.y()) / origH;
            break;
        case TransformHandle::BottomLeft:
            sx = (scaleAnchor_.x() - newPos.x()) / origW;
            sy = (newPos.y() - scaleAnchor_.y()) / origH;
            break;
        case TransformHandle::Left:
            sx = (scaleAnchor_.x() - newPos.x()) / origW;
            break;
        default:
            return;
    }

    // Clamp to minimum size (10% of original)
    sx = std::max(0.1, sx);
    sy = std::max(0.1, sy);

    if (proportional) {
        // For corner handles, use the average; for edge handles, only one dimension changed
        if (activeHandle_ == TransformHandle::TopLeft ||
            activeHandle_ == TransformHandle::TopRight ||
            activeHandle_ == TransformHandle::BottomLeft ||
            activeHandle_ == TransformHandle::BottomRight) {
            qreal avgScale = std::max(sx, sy);
            sx = avgScale;
            sy = avgScale;
        }
    }

    // Calculate the new top-left position to keep anchor fixed
    // The anchor is at a fixed position. After scaling, the original anchor corner
    // should map to the same screen position.
    qreal scaledW = origW * sx;
    qreal scaledH = origH * sy;

    qreal newLeft = 0;
    qreal newTop = 0;

    // Determine new top-left based on which corner/edge is anchored
    switch (activeHandle_) {
        case TransformHandle::TopLeft:
        case TransformHandle::Top:
        case TransformHandle::Left:
            // Anchor is on bottom-right side
            newLeft = scaleAnchor_.x() - scaledW;
            newTop = scaleAnchor_.y() - scaledH;
            if (activeHandle_ == TransformHandle::Top) {
                newLeft = originalBounds_.left() + translation_.x();  // X unchanged
            }
            if (activeHandle_ == TransformHandle::Left) {
                newTop = originalBounds_.top() + translation_.y();  // Y unchanged
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
            // Anchor is on top-left side
            newLeft = scaleAnchor_.x();
            newTop = scaleAnchor_.y();
            if (activeHandle_ == TransformHandle::Bottom) {
                newLeft = originalBounds_.left() + translation_.x();  // X unchanged
            }
            if (activeHandle_ == TransformHandle::Right) {
                newTop = originalBounds_.top() + translation_.y();  // Y unchanged
            }
            break;
        default:
            newLeft = originalBounds_.left();
            newTop = originalBounds_.top();
            break;
    }

    translation_ = QPointF(newLeft - originalBounds_.left(), newTop - originalBounds_.top());
    scale_ = QSizeF(sx, sy);

    spdlog::debug("[TransformState] Handle drag: scale=({:.2f},{:.2f}) offset=({:.1f},{:.1f})",
                  scale_.width(),
                  scale_.height(),
                  translation_.x(),
                  translation_.y());
}

void TransformState::updateFromDrag(const QPointF& delta)
{
    translation_ += delta;
}

}  // namespace gimp
