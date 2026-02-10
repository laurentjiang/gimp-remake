/**
 * @file transform_state.h
 * @brief Encapsulates transformation state for move/scale/rotate operations.
 * @author Laurent Jiang
 * @date 2026-02-10
 */

#pragma once

#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QSizeF>
#include <QTransform>

#include <vector>

namespace gimp {

/**
 * @brief Transform handle positions.
 */
enum class TransformHandle {
    None,         ///< No handle (drag to move)
    TopLeft,      ///< Top-left corner
    Top,          ///< Top edge center
    TopRight,     ///< Top-right corner
    Right,        ///< Right edge center
    BottomRight,  ///< Bottom-right corner
    Bottom,       ///< Bottom edge center
    BottomLeft,   ///< Bottom-left corner
    Left          ///< Left edge center
};

/**
 * @brief Encapsulates transformation state for move/scale/rotate operations.
 *
 * TransformState manages the complete transformation of a region, using QTransform
 * internally for unified handling of:
 * - Translation (move)
 * - Uniform/non-uniform scaling
 * - Rotation (future)
 * - Skew (future)
 *
 * The class provides methods to:
 * - Track original bounds and current transform
 * - Compute transformed corner/edge positions for handles
 * - Update transform based on handle drag operations
 * - Apply transform to pixel buffers
 */
class TransformState {
  public:
    TransformState() = default;

    /**
     * @brief Initializes transform state with original bounds.
     * @param bounds The original bounding rectangle before any transformation.
     */
    explicit TransformState(const QRectF& bounds);

    /// @brief Returns true if no transformation is active.
    [[nodiscard]] bool isIdentity() const;

    /// @brief Resets to identity transform.
    void reset();

    /// @brief Sets the original bounds (before transformation).
    void setOriginalBounds(const QRectF& bounds);

    /// @brief Returns the original bounds.
    [[nodiscard]] QRectF originalBounds() const { return originalBounds_; }

    /// @brief Returns the current translation offset.
    [[nodiscard]] QPointF translation() const { return translation_; }

    /// @brief Returns the current scale factors.
    [[nodiscard]] QSizeF scale() const { return scale_; }

    /// @brief Returns the current rotation angle (degrees).
    [[nodiscard]] qreal rotation() const { return rotation_; }

    /// @brief Returns the combined transform matrix.
    [[nodiscard]] QTransform matrix() const;

    /// @brief Returns the transformed bounding rectangle.
    [[nodiscard]] QRectF transformedBounds() const;

    /**
     * @brief Applies translation.
     * @param offset Translation offset.
     */
    void translate(const QPointF& offset);

    /**
     * @brief Sets the translation directly (replaces current translation).
     * @param offset New translation offset.
     */
    void setTranslation(const QPointF& offset);

    /**
     * @brief Applies uniform scaling relative to an anchor point.
     * @param factor Scale factor.
     * @param anchor Anchor point (remains fixed during scale).
     */
    void scaleUniform(qreal factor, const QPointF& anchor);

    /**
     * @brief Applies non-uniform scaling relative to an anchor point.
     * @param sx Horizontal scale factor.
     * @param sy Vertical scale factor.
     * @param anchor Anchor point (remains fixed during scale).
     */
    void scaleNonUniform(qreal sx, qreal sy, const QPointF& anchor);

    /**
     * @brief Sets the scale directly.
     * @param scale New scale factors.
     */
    void setScale(const QSizeF& scale);

    /**
     * @brief Applies rotation relative to an anchor point.
     * @param degrees Rotation angle in degrees.
     * @param anchor Anchor point (center of rotation).
     */
    void rotate(qreal degrees, const QPointF& anchor);

    /**
     * @brief Sets the rotation directly.
     * @param degrees New rotation angle.
     */
    void setRotation(qreal degrees);

    /**
     * @brief Returns the 8 handle rectangles in transformed coordinates.
     * @param handleSize Size of each handle in canvas pixels.
     * @return Vector of handle rects (order matches TransformHandle enum, excluding None).
     */
    [[nodiscard]] std::vector<QRectF> getHandleRects(qreal handleSize) const;

    /**
     * @brief Hit-tests for a transform handle at the given position.
     * @param pos Canvas position to test.
     * @param handleSize Size of each handle in canvas pixels.
     * @return The handle at that position, or None.
     */
    [[nodiscard]] TransformHandle hitTestHandle(const QPointF& pos, qreal handleSize) const;

    /**
     * @brief Returns the anchor point for the given handle (opposite corner/edge).
     * @param handle The handle being dragged.
     * @return Anchor point in canvas coordinates.
     */
    [[nodiscard]] QPointF getAnchorForHandle(TransformHandle handle) const;

    /**
     * @brief Updates transform based on handle drag.
     *
     * Calculates the new scale/translation based on dragging a handle
     * from its current position to a new position, keeping the anchor fixed.
     *
     * @param handle The handle being dragged.
     * @param newPos New handle position in canvas coordinates.
     * @param proportional If true, constrain to uniform scaling.
     */
    void updateFromHandleDrag(TransformHandle handle, const QPointF& newPos, bool proportional);

    /**
     * @brief Updates transform based on free drag (translation only).
     * @param delta Movement delta in canvas coordinates.
     */
    void updateFromDrag(const QPointF& delta);

  private:
    QRectF originalBounds_;              ///< Original bounds before transformation.
    QPointF translation_{0.0, 0.0};      ///< Current translation offset.
    QSizeF scale_{1.0, 1.0};             ///< Current scale factors.
    qreal rotation_ = 0.0;               ///< Current rotation angle (degrees).
    TransformHandle activeHandle_ = TransformHandle::None;  ///< Currently active handle.
    QPointF scaleAnchor_;                ///< Anchor point during scale operation.

    /// @brief Rebuilds transform matrix from components.
    [[nodiscard]] QTransform buildMatrix() const;

    /// @brief Returns corner/edge positions from transformed bounds.
    [[nodiscard]] std::vector<QPointF> getHandlePositions() const;
};

}  // namespace gimp
