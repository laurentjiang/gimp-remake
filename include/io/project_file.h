/**
 * @file project_file.h
 * @brief gimp::ProjectFile class definition.
 * @author Aless Tosi
 * @date 2026-01-08
 */

#pragma once

#include "core/document.h"

#include <memory>
#include <string>

namespace gimp {

/*!
 * @class ProjectFile
 * @brief Concrete implementation of Document for saveable project files.
 *
 * Manages layers and provides serialization support for project import/export.
 */
class ProjectFile : public Document {
  public:
    /*!
     * @brief Constructs a new project with the given dimensions.
     * @param w Canvas width in pixels.
     * @param h Canvas height in pixels.
     */
    ProjectFile(int w, int h) : m_width(w), m_height(h) {}

    ~ProjectFile() override = default;

    /*!
     * @brief Adds a new layer to the project.
     * @return Shared pointer to the newly created layer.
     */
    std::shared_ptr<gimp::Layer> add_layer() override
    {
        auto layer = std::make_shared<gimp::Layer>(m_width, m_height);
        m_layers.add_layer(layer);
        return layer;
    }

    /*!
     * @brief Removes a layer from the project.
     * @param layer The layer to remove.
     */
    void remove_layer(const std::shared_ptr<gimp::Layer>& layer) override
    {
        m_layers.remove_layer(layer);
    }

    /*! @brief Returns the layer stack.
     *  @return Const reference to the layer stack.
     */
    [[nodiscard]] const gimp::LayerStack& layers() const override { return m_layers; }

    /*! @brief Returns the tile store for dirty region tracking.
     *  @return Reference to the tile store.
     */
    gimp::TileStore& tile_store() override { return m_dummyTileStore; }

    /*! @brief Returns the canvas width in pixels.
     *  @return Width in pixels.
     */
    [[nodiscard]] int width() const override { return m_width; }

    /*! @brief Returns the canvas height in pixels.
     *  @return Height in pixels.
     */
    [[nodiscard]] int height() const override { return m_height; }

  private:
    int m_width;                ///< Canvas width.
    int m_height;               ///< Canvas height.
    gimp::LayerStack m_layers;  ///< Layer stack.

    /*! @brief Placeholder TileStore that does nothing. */
    class DummyTileStore : public TileStore {
        void invalidate(const gimp::Rect&) override {}
    } m_dummyTileStore;
};  // class ProjectFile
}  // namespace gimp
