/**
 * @file skia_renderer.h
 * @brief Skia-based implementation of the Renderer interface.
 * @author Laurent Jiang
 * @date 2025-12-15
 */

#pragma once

#include "renderer.h"
#include "skia_compositor.h"

#include <include/core/SkImage.h>
#include <include/core/SkSurface.h>

namespace gimp {
class Document;

/*!
 * @class SkiaRenderer
 * @brief Renders documents using the Skia graphics library.
 */
class SkiaRenderer : public Renderer {
  public:
    SkiaRenderer();
    ~SkiaRenderer() override;

    /*!
     * @brief Renders the document to an internal surface.
     * @param document The document to render.
     * @pre document.width() > 0 and document.height() > 0.
     * @post get_result() returns the rendered image.
     */
    void render(const Document& document) override;

    /*!
     * @brief Returns the rendered result as a Skia image.
     * @return The composited image, or nullptr if render() was not called.
     * @pre render() must have been called at least once.
     */
    sk_sp<SkImage> get_result();

  private:
    SkiaCompositor m_compositor;  ///< Compositor for layer blending.
    sk_sp<SkSurface> m_surface;   ///< Offscreen render surface.
};
}  // namespace gimp
