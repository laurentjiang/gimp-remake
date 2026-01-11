/**
 * @file skia_renderer.cpp
 * @brief Implementation of SkiaRenderer.
 * @author Laurent Jiang
 * @date 2025-12-15
 */

#include "skia_renderer.h"
#include <include/core/SkCanvas.h>
#include <iostream>
#include "../core/document.h"

namespace gimp {

SkiaRenderer::SkiaRenderer() = default;

SkiaRenderer::~SkiaRenderer() = default;

void SkiaRenderer::render(const Document& document)
{
    const int w = document.width();
    const int h = document.height();

    if (w <= 0 || h <= 0)
        return;

    if (!m_surface || m_surface->width() != w || m_surface->height() != h) {
        const SkImageInfo info = SkImageInfo::MakeN32Premul(w, h);
        m_surface = SkSurfaces::Raster(info);
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
