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
};

}  // namespace gimp
