/**
 * @file renderer.h
 * @brief Renderer interface to draw a document.
 * @author Laurent Jiang
 * @date 2025-12-08
 */

#pragma once

namespace gimp
{
class Document;

class Renderer
{
public:
    virtual ~Renderer() = default;
    virtual void render(const Document& document) = 0;
};
} // namespace gimp
