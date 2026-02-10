/**
 * @file document.h
 * @brief Minimal document interface for the image model.
 * @author Laurent Jiang
 * @date 2025-12-08
 */

#pragma once

#include "layer_stack.h"
#include "tile_store.h"

#include <QPainterPath>

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
    virtual std::shared_ptr<Layer> addLayer() = 0;

    /*!
     * @brief Removes a layer from the document.
     * @param layer The layer to remove.
     * @pre layer must exist in the document.
     * @post layers().count() is decremented by 1.
     */
    virtual void removeLayer(const std::shared_ptr<Layer>& layer) = 0;

    /*! @brief Returns the layer stack (const).
     *  @return Const reference to the document's layer stack.
     */
    [[nodiscard]] virtual const LayerStack& layers() const = 0;

    /*! @brief Returns the layer stack (mutable).
     *  @return Mutable reference to the document's layer stack.
     */
    virtual LayerStack& layers() = 0;

    /*! @brief Returns the currently active layer.
     *  @return Shared pointer to the active layer, or nullptr if no layers.
     */
    [[nodiscard]] virtual std::shared_ptr<Layer> activeLayer() const = 0;

    /*! @brief Returns the index of the currently active layer.
     *  @return Index in the layer stack (0-based).
     */
    [[nodiscard]] virtual std::size_t activeLayerIndex() const = 0;

    /*! @brief Sets the active layer by index.
     *  @param index The layer index to make active (clamped to valid range).
     */
    virtual void setActiveLayerIndex(std::size_t index) = 0;

    /*! @brief Returns the tile store for dirty region tracking.
     *  @return Reference to the tile store.
     */
    virtual TileStore& tileStore() = 0;

    /*! @brief Returns the document width in pixels.
     *  @return Width in pixels.
     */
    [[nodiscard]] virtual int width() const = 0;

    /*! @brief Returns the document height in pixels.
     *  @return Height in pixels.
     */
    [[nodiscard]] virtual int height() const = 0;

    /*! @brief Sets the document selection path.
     *  @param path The selection path in canvas coordinates.
     */
    virtual void setSelectionPath(const QPainterPath& path) = 0;

    /*! @brief Returns the document selection path.
     *  @return The selection path.
     */
    [[nodiscard]] virtual QPainterPath selectionPath() const = 0;
};  // class Document
}  // namespace gimp
