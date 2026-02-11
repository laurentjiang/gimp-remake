/**
 * @file project_file.h
 * @brief gimp::ProjectFile class definition.
 * @author Aless Tosi
 * @date 2026-01-08
 */

#pragma once

#include "core/document.h"

#include <QPainterPath>

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
    * @param dpi Resolution in DPI.
     */
    ProjectFile(int w, int h, double dpi = 72.0) : m_width(w), m_height(h), m_dpi(dpi) {}

    ~ProjectFile() override = default;

    /*!
     * @brief Adds a new layer to the project.
     * @return Shared pointer to the newly created layer.
     */
    std::shared_ptr<gimp::Layer> addLayer() override { return addLayer(m_width, m_height); }

    /*!
     * @brief Adds a new layer with custom dimensions.
     * @param width Layer width in pixels.
     * @param height Layer height in pixels.
     * @return Shared pointer to the newly created layer.
     */
    std::shared_ptr<gimp::Layer> addLayer(int width, int height)
    {
        auto layer = std::make_shared<gimp::Layer>(width, height);
        layer->setName("Layer " + std::to_string(++m_layerCounter));
        m_layers.addLayer(layer);
        return layer;
    }

    /*!
     * @brief Removes a layer from the project.
     * @param layer The layer to remove.
     */
    void removeLayer(const std::shared_ptr<gimp::Layer>& layer) override
    {
        // Find index of layer being removed to adjust active index
        std::size_t removedIndex = m_layers.count();  // invalid sentinel
        for (std::size_t i = 0; i < m_layers.count(); ++i) {
            if (m_layers[i] == layer) {
                removedIndex = i;
                break;
            }
        }

        m_layers.removeLayer(layer);

        // Adjust active layer index if needed
        if (!m_layers.empty()) {
            if (m_activeLayerIndex >= m_layers.count()) {
                m_activeLayerIndex = m_layers.count() - 1;
            } else if (removedIndex < m_activeLayerIndex) {
                --m_activeLayerIndex;
            }
        } else {
            m_activeLayerIndex = 0;
        }
    }

    /*! @brief Returns the layer stack (const).
     *  @return Const reference to the layer stack.
     */
    [[nodiscard]] const gimp::LayerStack& layers() const override { return m_layers; }

    /*! @brief Returns the layer stack (mutable).
     *  @return Mutable reference to the layer stack.
     */
    gimp::LayerStack& layers() override { return m_layers; }

    /*! @brief Returns the currently active layer.
     *  @return Shared pointer to the active layer, or nullptr if no layers.
     */
    [[nodiscard]] std::shared_ptr<gimp::Layer> activeLayer() const override
    {
        if (m_layers.empty()) {
            return nullptr;
        }
        return m_layers[m_activeLayerIndex];
    }

    /*! @brief Returns the index of the currently active layer.
     *  @return Index in the layer stack (0-based).
     */
    [[nodiscard]] std::size_t activeLayerIndex() const override { return m_activeLayerIndex; }

    /*! @brief Sets the active layer by index.
     *  @param index The layer index to make active (clamped to valid range).
     */
    void setActiveLayerIndex(std::size_t index) override
    {
        if (m_layers.empty()) {
            m_activeLayerIndex = 0;
            return;
        }
        m_activeLayerIndex = std::min(index, m_layers.count() - 1);
    }

    /*! @brief Resets the layer counter to 0.
     *  Use after creating the background layer so next layer is "Layer 1".
     */
    void resetLayerCounter() { m_layerCounter = 0; }

    /*! @brief Returns the tile store for dirty region tracking.
     *  @return Reference to the tile store.
     */
    gimp::TileStore& tileStore() override { return m_dummyTileStore; }

    /*! @brief Returns the canvas width in pixels.
     *  @return Width in pixels.
     */
    [[nodiscard]] int width() const override { return m_width; }

    /*! @brief Returns the canvas height in pixels.
     *  @return Height in pixels.
     */
    [[nodiscard]] int height() const override { return m_height; }

    /*! @brief Returns the document resolution in DPI.
     *  @return Resolution in DPI.
     */
    [[nodiscard]] double dpi() const { return m_dpi; }

    /*! @brief Sets the document resolution in DPI.
     *  @param dpi Resolution in DPI.
     */
    void setDpi(double dpi) { m_dpi = dpi; }

    /*! @brief Sets the document selection path.
     *  @param path The selection path in canvas coordinates.
     */
    void setSelectionPath(const QPainterPath& path) override { selection_ = path; }

    /*! @brief Returns the document selection path.
     *  @return The selection path.
     */
    [[nodiscard]] QPainterPath selectionPath() const override { return selection_; }

  private:
    int m_width;                ///< Canvas width.
    int m_height;               ///< Canvas height.
    double m_dpi;               ///< Resolution in DPI.
    std::size_t m_activeLayerIndex = 0;  ///< Index of the active layer.
    int m_layerCounter = 0;              ///< Counter for auto-incrementing layer names.
    gimp::LayerStack m_layers;           ///< Layer stack.
    QPainterPath selection_;             ///< Stored selection path.

    /*! @brief Placeholder TileStore that does nothing. */
    class DummyTileStore : public TileStore {
        void invalidate(const gimp::Rect&) override {}
    } m_dummyTileStore;
};  // class ProjectFile
}  // namespace gimp
