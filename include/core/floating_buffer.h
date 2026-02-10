/**
 * @file floating_buffer.h
 * @brief Encapsulates extracted selection pixels for move/transform operations.
 * @author Laurent Jiang
 * @date 2026-02-10
 */

#pragma once

#include <QPainterPath>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QSizeF>

#include <cstdint>
#include <memory>
#include <vector>

namespace gimp {

class Layer;

/**
 * @brief Encapsulates extracted selection pixels for move/transform operations.
 *
 * The FloatingBuffer holds a copy of pixels extracted from a selection region,
 * along with a pre-rasterized selection mask for efficient per-pixel operations.
 * This enables:
 * - Efficient extraction of irregular selection shapes
 * - Preview rendering during drag operations
 * - Scaling operations before commit
 * - Clear undo/redo boundaries
 */
class FloatingBuffer {
  public:
    FloatingBuffer() = default;

    /// @brief Returns true if the buffer is empty (no extracted pixels).
    [[nodiscard]] bool isEmpty() const { return buffer_.empty(); }

    /// @brief Clears all buffer data and resets state.
    void clear();

    /// @brief Returns the source bounding rectangle.
    [[nodiscard]] QRect sourceRect() const { return sourceRect_; }

    /// @brief Returns the pixel buffer (RGBA, 4 bytes per pixel).
    [[nodiscard]] const std::vector<std::uint8_t>& data() const { return buffer_; }

    /// @brief Returns the width of the buffer.
    [[nodiscard]] int width() const { return sourceRect_.width(); }

    /// @brief Returns the height of the buffer.
    [[nodiscard]] int height() const { return sourceRect_.height(); }

    /**
     * @brief Extracts pixels from a layer using the given selection path.
     *
     * Rasterizes the selection mask and copies pixels from the layer into
     * the internal buffer. Only pixels inside the selection are copied;
     * others remain transparent.
     *
     * @param layer The layer to extract from.
     * @param selectionPath The selection path defining which pixels to extract.
     * @return True if extraction succeeded (valid selection with pixels).
     */
    bool extractFromLayer(const std::shared_ptr<Layer>& layer, const QPainterPath& selectionPath);

    /**
     * @brief Clears source pixels in the layer to transparent.
     *
     * Uses the pre-rasterized selection mask to clear only selected pixels.
     * Call this after extractFromLayer() for cut operations.
     *
     * @param layer The layer to modify.
     */
    void clearSourcePixels(const std::shared_ptr<Layer>& layer);

    /**
     * @brief Pastes buffer pixels back to a layer at the given offset.
     *
     * Uses the pre-rasterized selection mask to paste only selected pixels.
     * Clips to layer bounds automatically.
     *
     * @param layer The layer to paste to.
     * @param offset Offset from original source position.
     */
    void pasteToLayer(const std::shared_ptr<Layer>& layer, QPoint offset);

    /**
     * @brief Creates a scaled copy of the buffer.
     *
     * Uses bilinear interpolation for smooth scaling.
     *
     * @param scale Scale factors (width, height).
     * @return Scaled pixel data (RGBA).
     */
    [[nodiscard]] std::vector<std::uint8_t> getScaled(QSizeF scale) const;

    /**
     * @brief Returns the scaled dimensions.
     *
     * @param scale Scale factors.
     * @return Scaled size in pixels.
     */
    [[nodiscard]] QSize getScaledSize(QSizeF scale) const;

    /**
     * @brief Checks if a pixel at buffer coordinates is selected.
     *
     * @param col Column (0 to width-1).
     * @param row Row (0 to height-1).
     * @return True if the pixel is inside the selection.
     */
    [[nodiscard]] bool isPixelSelected(int col, int row) const;

  private:
    /**
     * @brief Pre-rasterizes the selection path into a boolean mask.
     *
     * @param selPath The selection path to rasterize.
     * @param bounds The bounding rectangle for the mask.
     */
    void rasterizeSelectionMask(const QPainterPath& selPath, const QRect& bounds);

    std::vector<std::uint8_t> buffer_;  ///< Extracted pixel data (RGBA).
    QRect sourceRect_;                  ///< Source bounding rectangle.
    std::vector<bool> selectionMask_;   ///< Pre-rasterized selection mask.
};

}  // namespace gimp
