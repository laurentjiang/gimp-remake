/**
 * @file skia_renderer.cpp
 * @brief Implementation of SkiaRenderer.
 * @author Laurent Jiang
 * @date 2025-12-15
 */

#include "render/skia_renderer.h"

#include "core/document.h"
#include "render/gpu_context.h"

#include <iostream>

#include <gpu/ganesh/SkSurfaceGanesh.h>
#include <include/core/SkCanvas.h>

namespace gimp {

SkiaRenderer::SkiaRenderer() = default;

SkiaRenderer::~SkiaRenderer() = default;

void SkiaRenderer::setGpuContext(IGpuContext* gpuContext)
{
    m_gpuContext = gpuContext;
    // Invalidate existing surfaces so they get recreated with correct backend
    m_surface.reset();
    m_partialSurface.reset();
}

bool SkiaRenderer::isUsingGpu() const
{
    return m_useGpu;
}

SkSurface* SkiaRenderer::surface() const
{
    return m_surface.get();
}

bool SkiaRenderer::ensureSurface(sk_sp<SkSurface>& surface, int width, int height)
{
    // Check if existing surface is valid and correct size
    if (surface && surface->width() == width && surface->height() == height) {
        return true;
    }

    const SkImageInfo info = SkImageInfo::MakeN32Premul(width, height);

    // Try GPU surface if context is available and valid
    if (m_gpuContext && m_gpuContext->isValid()) {
        surface = SkSurfaces::RenderTarget(m_gpuContext->grContext(), skgpu::Budgeted::kNo, info);
        if (surface) {
            m_useGpu = true;
            return true;
        }
        // Fall through to raster if GPU surface creation fails
    }

    // Fallback to CPU raster surface
    surface = SkSurfaces::Raster(info);
    m_useGpu = false;
    return surface != nullptr;
}

void SkiaRenderer::render(const Document& document)
{
    const int w = document.width();
    const int h = document.height();

    if (w <= 0 || h <= 0)
        return;

    if (!ensureSurface(m_surface, w, h))
        return;

    SkCanvas* canvas = m_surface->getCanvas();
    canvas->clear(SK_ColorTRANSPARENT);

    m_compositor.compose(canvas, document.layers());
}

sk_sp<SkImage> SkiaRenderer::renderBelow(const Document& document, std::size_t activeLayerIndex)
{
    const int w = document.width();
    const int h = document.height();

    if (w <= 0 || h <= 0 || activeLayerIndex == 0) {
        return nullptr;  // No layers below index 0
    }

    if (!ensureSurface(m_partialSurface, w, h)) {
        return nullptr;
    }

    SkCanvas* canvas = m_partialSurface->getCanvas();
    canvas->clear(SK_ColorTRANSPARENT);

    m_compositor.composeUpTo(canvas, document.layers(), activeLayerIndex);

    return m_partialSurface->makeImageSnapshot();
}

sk_sp<SkImage> SkiaRenderer::renderActiveLayerOnly(const Document& document)
{
    const int w = document.width();
    const int h = document.height();

    if (w <= 0 || h <= 0) {
        return nullptr;
    }

    auto activeLayer = document.activeLayer();
    if (!activeLayer) {
        return nullptr;
    }

    if (!ensureSurface(m_partialSurface, w, h)) {
        return nullptr;
    }

    SkCanvas* canvas = m_partialSurface->getCanvas();
    canvas->clear(SK_ColorTRANSPARENT);

    m_compositor.composeSingleLayer(canvas, *activeLayer);

    return m_partialSurface->makeImageSnapshot();
}

sk_sp<SkImage> SkiaRenderer::get_result()
{
    if (m_surface) {
        return m_surface->makeImageSnapshot();
    }
    return nullptr;
}

}  // namespace gimp
