/**
 * @file skia_renderer.cpp
 * @brief Implementation of SkiaRenderer.
 * @author Laurent Jiang
 * @date 2025-12-15
 */

#include "skia_renderer.h"
#include "../core/document.h"
#include <iostream>

namespace gimp
{

SkiaRenderer::SkiaRenderer()
{
}

SkiaRenderer::~SkiaRenderer() = default;

void SkiaRenderer::render(const Document& document)
{
    (void)document;
}

} // namespace gimp
