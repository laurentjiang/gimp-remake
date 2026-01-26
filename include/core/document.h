/**
 * @file document.h
 * @brief Minimal document interface for the image model.
 * @author Laurent Jiang
 * @date 2025-12-08
 */

#pragma once

#include "layer_stack.h"
#include "tile_store.h"

#include <memory>
#include <vector>

namespace gimp {

/*!
 * @class Document
 * @brief Abstract interface for an image document with layers.
 */
class Document {
  public:
    virtual ~Document() = default;

    /*!
     * @brief Adds a new layer to the document.
     * @return Shared pointer to the created layer.
     * @post layers().count() is incremented by 1.
     * @post Returned pointer is non-null.
     */
    virtual std::shared_ptr<Layer> add_layer() = 0;

    /*!
     * @brief Removes a layer from the document.
     * @param layer The layer to remove.
     * @pre layer must exist in the document.
     * @post layers().count() is decremented by 1.
     */
    virtual void remove_layer(const std::shared_ptr<Layer>& layer) = 0;

    /*! @brief Returns the layer stack.
     *  @return Reference to the document's layer stack.
     */
    virtual const LayerStack& layers() const = 0;

    /*! @brief Returns the tile store for dirty region tracking.
     *  @return Reference to the tile store.
     */
    virtual TileStore& tile_store() = 0;

    /*! @brief Returns the document width in pixels.
     *  @return Width in pixels.
     */
    virtual int width() const = 0;

    /*! @brief Returns the document height in pixels.
     *  @return Height in pixels.
     */
    virtual int height() const = 0;
};  // class Document
}  // namespace gimp
