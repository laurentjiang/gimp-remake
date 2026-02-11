/**
 * @file project_file.h
 * @brief gimp::ProjectFile class definition.
 * @author Aless Tosi
 * @date 2026-01-08
 */

#pragma once

#include "core/document.h"

#include <QPainterPath>

#include <filesystem>
#include <memory>
#include <optional>
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
    std::shared_ptr<gimp::Layer> addLayer() override
    {
        auto layer = std::make_shared<gimp::Layer>(m_width, m_height);
        m_layers.addLayer(layer);
        return layer;
    }

    /*!
     * @brief Removes a layer from the project.
     * @param layer The layer to remove.
     */
    void removeLayer(const std::shared_ptr<gimp::Layer>& layer) override
    {
        m_layers.removeLayer(layer);
    }

    /*! @brief Returns the layer stack.
     *  @return Const reference to the layer stack.
     */
    [[nodiscard]] const gimp::LayerStack& layers() const override { return m_layers; }

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

    /*! @brief Sets the document selection path.
     *  @param path The selection path in canvas coordinates.
     */
    void setSelectionPath(const QPainterPath& path) override { selection_ = path; }

    /*! @brief Returns the document selection path.
     *  @return The selection path.
     */
    [[nodiscard]] QPainterPath selectionPath() const override { return selection_; }

    /*! @brief Sets the file path associated with this project.
     *  @param path The file path.
     */
    void setFilePath(const std::filesystem::path& path) { m_filePath = path; }

    /*! @brief Returns the file path associated with this project.
     *  @return Optional file path (empty if never saved).
     */
    [[nodiscard]] std::optional<std::filesystem::path> filePath() const { return m_filePath; }

  private:
    int m_width;                                      ///< Canvas width.
    int m_height;                                     ///< Canvas height.
    gimp::LayerStack m_layers;                        ///< Layer stack.
    QPainterPath selection_;                          ///< Stored selection path.
    std::optional<std::filesystem::path> m_filePath;  ///< Associated file path.

    /*! @brief Placeholder TileStore that does nothing. */
    class DummyTileStore : public TileStore {
        void invalidate(const gimp::Rect&) override {}
    } m_dummyTileStore;
};  // class ProjectFile
}  // namespace gimp
