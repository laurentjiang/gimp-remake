/**
 * @file skia_renderer.h
 * @brief Skia-based implementation of the Renderer interface.
 * @author Laurent Jiang
 * @date 2025-12-15
 */

#pragma once

#include <include/core/SkImage.h>
#include <include/core/SkSurface.h>
#include "renderer.h"
#include "skia_compositor.h"

namespace gimp {
class Document;

class SkiaRenderer : public Renderer {
  public:
    SkiaRenderer();
    ~SkiaRenderer() override;

    void render(const Document& document) override;

    sk_sp<SkImage> get_result();

  private:
    SkiaCompositor m_compositor;
    sk_sp<SkSurface> m_surface;
};
}  // namespace gimp
