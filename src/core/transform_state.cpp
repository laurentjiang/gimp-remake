/**
 * @file transform_state.cpp
 * @brief Implementation of TransformState.
 * @author Laurent Jiang
 * @date 2026-02-10
 */

#include "core/transform_state.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>

namespace gimp {

TransformState::TransformState(const QRectF& bounds) : originalBounds_(bounds)
{
}

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
    return matrix().mapRect(originalBounds_);
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
    QPointF newOffset = QPointF(offset.x() * sx / scale_.width(), offset.y() * sy / scale_.height());
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
    positions.push_back(QPointF(left, top));
    positions.push_back(QPointF(midX, top));
    positions.push_back(QPointF(right, top));
    positions.push_back(QPointF(right, midY));
    positions.push_back(QPointF(right, bottom));
    positions.push_back(QPointF(midX, bottom));
    positions.push_back(QPointF(left, bottom));
    positions.push_back(QPointF(left, midY));

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
        handles.push_back(
            QRectF(pos.x() - halfSize, pos.y() - halfSize, handleSize, handleSize));
    }

    return handles;
}

TransformHandle TransformState::hitTestHandle(const QPointF& pos, qreal handleSize) const
{
    auto handles = getHandleRects(handleSize);
    if (handles.empty()) {
        return TransformHandle::None;
    }

    static const TransformHandle handleTypes[] = {TransformHandle::TopLeft,
                                                   TransformHandle::Top,
                                                   TransformHandle::TopRight,
                                                   TransformHandle::Right,
                                                   TransformHandle::BottomRight,
                                                   TransformHandle::Bottom,
                                                   TransformHandle::BottomLeft,
                                                   TransformHandle::Left};

    for (std::size_t i = 0; i < handles.size(); ++i) {
        if (handles[i].contains(pos)) {
            return handleTypes[i];
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

void TransformState::updateFromHandleDrag(TransformHandle handle,
                                          const QPointF& newPos,
                                          bool proportional)
{
    if (originalBounds_.isEmpty() || handle == TransformHandle::None) {
        return;
    }

    QPointF anchor = getAnchorForHandle(handle);
    QRectF currentBounds = transformedBounds();

    // Calculate new bounds based on which handle is being dragged
    qreal newLeft = currentBounds.left();
    qreal newTop = currentBounds.top();
    qreal newRight = currentBounds.right();
    qreal newBottom = currentBounds.bottom();

    switch (handle) {
        case TransformHandle::TopLeft:
            newLeft = newPos.x();
            newTop = newPos.y();
            break;
        case TransformHandle::Top:
            newTop = newPos.y();
            break;
        case TransformHandle::TopRight:
            newRight = newPos.x();
            newTop = newPos.y();
            break;
        case TransformHandle::Right:
            newRight = newPos.x();
            break;
        case TransformHandle::BottomRight:
            newRight = newPos.x();
            newBottom = newPos.y();
            break;
        case TransformHandle::Bottom:
            newBottom = newPos.y();
            break;
        case TransformHandle::BottomLeft:
            newLeft = newPos.x();
            newBottom = newPos.y();
            break;
        case TransformHandle::Left:
            newLeft = newPos.x();
            break;
        default:
            return;
    }

    // Normalize bounds (handle inverted dragging)
    QRectF newBounds =
        QRectF(QPointF(newLeft, newTop), QPointF(newRight, newBottom)).normalized();

    // Calculate scale factors relative to original
    qreal sx = originalBounds_.width() > 0.0 ? newBounds.width() / originalBounds_.width() : 1.0;
    qreal sy =
        originalBounds_.height() > 0.0 ? newBounds.height() / originalBounds_.height() : 1.0;

    if (proportional) {
        // Use the larger scale factor for uniform scaling
        qreal maxScale = std::max(std::abs(sx), std::abs(sy));
        sx = sx < 0 ? -maxScale : maxScale;
        sy = sy < 0 ? -maxScale : maxScale;
    }

    // Calculate translation so anchor remains fixed
    // New anchor position should equal original anchor position
    QPointF originalTopLeft = originalBounds_.topLeft();
    QPointF newTopLeft = anchor - QPointF(anchor.x() - originalTopLeft.x(),
                                           anchor.y() - originalTopLeft.y());

    // Recalculate based on scale from anchor
    QPointF scaledTopLeft =
        anchor + QPointF((originalBounds_.left() - anchor.x()) * sx / scale_.width(),
                         (originalBounds_.top() - anchor.y()) * sy / scale_.height());

    translation_ = newBounds.topLeft() - originalBounds_.topLeft();
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
