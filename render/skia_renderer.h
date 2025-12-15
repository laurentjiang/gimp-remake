/**
 * @file skia_renderer.h
 * @brief Skia-based implementation of the Renderer interface.
 * @author Laurent Jiang
 * @date 2025-12-15
 */

#pragma once

#include "renderer.h"

namespace gimp
{
class Document;

class SkiaRenderer : public Renderer
{
public:
    SkiaRenderer();
    ~SkiaRenderer() override;

    void render(const Document& document) override;
};
} // namespace gimp
