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
class IGpuContext;

/*!
 * @class SkiaRenderer
 * @brief Renders documents using the Skia graphics library.
 *
 * Supports both CPU raster and GPU-accelerated rendering. When a GPU context
 * is set and valid, surfaces are created on the GPU for hardware compositing.
 */
class SkiaRenderer : public Renderer {
  public:
    SkiaRenderer();
    ~SkiaRenderer() override;

    /*!
     * @brief Set the GPU context for hardware-accelerated rendering.
     * @param gpuContext Pointer to GPU context (or NullGpuContext for CPU fallback).
     */
    void setGpuContext(IGpuContext* gpuContext);

    /*!
     * @brief Check if GPU rendering is currently active.
     * @return true if using GPU surfaces, false if using CPU raster.
     */
    bool isUsingGpu() const;

    /*!
     * @brief Get the current render surface.
     * @return Pointer to the internal surface (for external flush/access).
     */
    SkSurface* surface() const;

    /*!
     * @brief Renders the document to an internal surface.
     * @param document The document to render.
     * @pre document.width() > 0 and document.height() > 0.
     * @post get_result() returns the rendered image.
     */
    void render(const Document& document) override;

    /*!
     * @brief Renders layers below the active layer.
     *
     * Used to build the below-layer cache for fast stroke rendering.
     *
     * @param document The document to render.
     * @param activeLayerIndex The active layer index (layers 0..activeLayerIndex-1 are rendered).
     * @return The composited image of layers below active, or nullptr on error.
     */
    sk_sp<SkImage> renderBelow(const Document& document, std::size_t activeLayerIndex);

    /*!
     * @brief Renders only the active layer.
     *
     * Used in combination with below-layer cache for fast stroke rendering.
     *
     * @param document The document to render.
     * @return The rendered active layer image, or nullptr on error.
     */
    sk_sp<SkImage> renderActiveLayerOnly(const Document& document);

    /*!
     * @brief Returns the rendered result as a Skia image.
     * @return The composited image, or nullptr if render() was not called.
     * @pre render() must have been called at least once.
     */
    sk_sp<SkImage> get_result();

  private:
    /*!
     * @brief Create or recreate a surface with the given dimensions.
     * @param surface The surface smart pointer to update.
     * @param width Desired width.
     * @param height Desired height.
     * @return true if surface is valid after the call.
     */
    bool ensureSurface(sk_sp<SkSurface>& surface, int width, int height);

    SkiaCompositor m_compositor;         ///< Compositor for layer blending.
    sk_sp<SkSurface> m_surface;          ///< Offscreen render surface for full renders.
    sk_sp<SkSurface> m_partialSurface;   ///< Offscreen surface for partial renders.
    IGpuContext* m_gpuContext = nullptr; ///< GPU context (never null after init, uses NullGpuContext).
    bool m_useGpu = false;               ///< Whether GPU rendering is currently active.
};
}  // namespace gimp
