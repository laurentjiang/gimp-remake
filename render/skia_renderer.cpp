/**
 * @file skia_renderer.cpp
 * @brief Implementation of SkiaRenderer.
 * @author Laurent Jiang
 * @date 2025-12-15
 */

#include "skia_renderer.H"

#include "../core/document.H"

#include <iostream>

#include <include/core/SkCanvas.H>

namespace gimp {

SkiaRenderer::SkiaRenderer() = default;

SkiaRenderer::~SkiaRenderer() = default;

void SkiaRenderer::render(const Document& document)
{
    const int W = document.width();
    const int H = document.height();

    if (W <= 0 || H <= 0)
        return;

    if (!m_surface || m_surface->width() != W || m_surface->height() != H) {
        const SkImageInfo INFO = SkImageInfo::MakeN32Premul(W, H);
        m_surface = SkSurfaces::Raster(INFO);
    }

    if (!m_surface)
        return;

    SkCanvas* canvas = m_surface->getCanvas();
    canvas->clear(SK_ColorTRANSPARENT);

    m_compositor.compose(canvas, document.layers());
}

sk_sp<SkImage> SkiaRenderer::get_result()
{
    if (m_surface) {
        return m_surface->makeImageSnapshot();
    }
    return nullptr;
}

}  // namespace gimp
