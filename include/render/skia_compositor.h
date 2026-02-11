/**
 * @file skia_compositor.h
 * @brief Compositor implementation using Skia.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#pragma once

#include "core/layer_stack.h"

class SkCanvas;

namespace gimp {

class Layer;

/*!
 * @class SkiaCompositor
 * @brief Composites layer stacks onto a Skia canvas.
 */
class SkiaCompositor {
  public:
    /*!
     * @brief Composites all visible layers onto the canvas.
     * @param canvas The Skia canvas to draw on.
     * @param layers The layer stack to composite.
     */
    void compose(SkCanvas* canvas, const LayerStack& layers);

    /*!
     * @brief Composites visible layers up to (but not including) the specified index.
     *
     * Used for caching layers below the active layer during brush strokes.
     *
     * @param canvas The Skia canvas to draw on.
     * @param layers The layer stack to composite.
     * @param stopBeforeIndex Stop compositing before this layer index.
     */
    void composeUpTo(SkCanvas* canvas, const LayerStack& layers, std::size_t stopBeforeIndex);

    /*!
     * @brief Composites a single layer onto the canvas.
     * @param canvas The Skia canvas to draw on.
     * @param layer The layer to composite.
     */
    void composeSingleLayer(SkCanvas* canvas, const Layer& layer);
};

}  // namespace gimp
