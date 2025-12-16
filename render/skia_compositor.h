/**
 * @file skia_compositor.h
 * @brief Compositor implementation using Skia.
 * @author Laurent Jiang
 * @date 2025-12-16
 */

#pragma once

#include "../core/layer_stack.h"

class SkCanvas;

namespace gimp {

class SkiaCompositor {
  public:
    void compose(SkCanvas* canvas, const LayerStack& layers);
};

}  // namespace gimp
