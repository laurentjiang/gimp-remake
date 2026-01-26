/**
 * @file renderer.h
 * @brief Renderer interface to draw a document.
 * @author Laurent Jiang
 * @date 2025-12-08
 */

#pragma once

namespace gimp {
class Document;

/*!
 * @class Renderer
 * @brief Abstract interface for rendering a document to an output.
 */
class Renderer {
  public:
    virtual ~Renderer() = default;

    /*!
     * @brief Renders the document.
     * @param document The document to render.
     */
    virtual void render(const Document& document) = 0;
};
}  // namespace gimp
