/**
 * @file image_file.h
 * @brief gimp::ProjectFile class definition.
 * @author Aless Tosi
 * @date 2025-01-08
 */

#pragma once

#include <string>
#include <memory>
#include "core/document.h"

namespace gimp {
    class ProjectFile : public Document {
        public:
            ProjectFile(int w, int h) : m_width(w), m_height(h) {}
            ~ProjectFile() override = default;

            std::shared_ptr<gimp::Layer> add_layer() override
            {
                auto layer = std::make_shared<gimp::Layer>(m_width, m_height);
                m_layers.add_layer(layer);
                return layer;
            }

            void remove_layer(const std::shared_ptr<gimp::Layer>& layer) override
            {
                m_layers.remove_layer(layer);
            }

            [[nodiscard]] const gimp::LayerStack& layers() const override { return m_layers; }

            gimp::TileStore& tile_store() override { return m_dummyTileStore; }

            [[nodiscard]] int width() const override { return m_width; }
            [[nodiscard]] int height() const override { return m_height; }

        private:
            int m_width;
            int m_height;
            gimp::LayerStack m_layers;

            class DummyTileStore : public TileStore {
                void invalidate(const gimp::Rect&) override {}
            } m_dummyTileStore;
    }; // class ProjectFile
}

